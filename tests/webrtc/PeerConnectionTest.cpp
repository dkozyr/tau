#include "tau/webrtc/PeerConnection.h"
#include "tau/http/Server.h"
#include "tau/http/Client.h"
#include "tau/rtp-session/FrameProcessor.h"
#include "tau/rtp-packetization/H264Depacketizer.h"
#include "tau/video/h264/Avc1NaluProcessor.h"
#include "tau/video/h264/AnnexB.h"
#include "tau/asio/ThreadPool.h"
#include "tau/common/File.h"
#include "tau/common/Json.h"
#include "tau/common/File.h"
#include "tau/common/Ntp.h"
#include "tests/lib/Common.h"

namespace tau::webrtc {

class PeerConnectionTest : public ::testing::Test {
public:
    PeerConnectionTest()
        : _io(std::thread::hardware_concurrency())
    {}

    ~PeerConnectionTest() {
        _io.Join();
    }

protected:
    std::string ProcessSdpOffer(const std::string& offer) {
        _pc.reset();
        _pc.emplace(
            PeerConnection::Dependencies{
                .clock = _clock,
                .executor = _io.GetStrand(),
                .udp_allocator = g_udp_allocator
            },
            PeerConnection::Options{
                .log_ctx = {}
            }
        );
        _ctx.emplace(PcContext{
            .frame_processor{},
            .h264_depacketizer = rtp::H264Depacketizer{g_system_allocator},
            .avc1_nalu_processor{h264::Avc1NaluProcessor::Options{}},
            .output_path = std::to_string(ToNtp(_clock.Now())) + ".h264"
        });
        _ctx->frame_processor.SetCallback([this](rtp::Frame&& frame, bool losses) {
            const auto ok = !losses && _ctx->h264_depacketizer.Process(std::move(frame));
            if(!ok) {
                TAU_LOG_INFO("Drop until key-frame, frame rtp packets: " << frame.size() << (losses ? ", losses" : ""));
                _ctx->avc1_nalu_processor.DropUntilKeyFrame();
            }
        });
        _ctx->h264_depacketizer.SetCallback([this](Buffer&& nal_unit) {
            _ctx->avc1_nalu_processor.Push(std::move(nal_unit));
        });
        _ctx->avc1_nalu_processor.SetCallback([this](Buffer&& nal_unit) {
            const auto header = reinterpret_cast<const h264::NaluHeader*>(&nal_unit.GetView().ptr[0]);
            TAU_LOG_INFO("[H264] [avc1] nal unit type: " << (size_t)header->type << ", tp: " << DurationSec(nal_unit.GetInfo().tp) << ", size: " << nal_unit.GetSize());
            auto view = nal_unit.GetView();
            //TODO: ToStringView
            WriteFile(_ctx->output_path, std::string_view{reinterpret_cast<const char*>(h264::kAnnexB.data()), h264::kAnnexB.size()}, true);
            WriteFile(_ctx->output_path, std::string_view{reinterpret_cast<const char*>(view.ptr), view.size}, true);
        });

        _pc->SetRecvRtpCallback([this](size_t idx, Buffer&& packet) {
            if(idx == 1) {
                _ctx->frame_processor.PushRtp(std::move(packet));
            }
        });
        return _pc->ProcessSdpOffer(offer);
    }

    void Process() {
        std::lock_guard lock{_mutex};
        if(_pc) {
            _pc->Process();
        }
    }

protected:
    ThreadPool _io;
    SteadyClock _clock;
    std::mutex _mutex;
    std::optional<PeerConnection> _pc;

    struct PcContext {
        rtp::session::FrameProcessor frame_processor;
        rtp::H264Depacketizer h264_depacketizer;
        h264::Avc1NaluProcessor avc1_nalu_processor;
        std::filesystem::path output_path;
    };
    std::optional<PcContext> _ctx;
};

TEST_F(PeerConnectionTest, DISABLED_MANUAL_Localhost) {
    crypto::Certificate ca(crypto::Certificate::Options{
        .cert = std::string{PROJECT_SOURCE_DIR} + "/data/ca.crt",
        .key  = std::string{PROJECT_SOURCE_DIR} + "/data/ca.key"
    });
    crypto::Certificate cert(crypto::Certificate::OptionsSelfSigned{.ca = ca});

    http::Server server(
        http::Server::Dependencies{.executor = _io.GetExecutor()},
        http::Server::Options{
            .port = 8888,
            .ssl_ctx = CreateSslContextPtr(cert.GetCertificateBuffer(), cert.GetPrivateKeyBuffer())
        });

    server.SetRequestCallback(
        [this](const beast_request& request, const http::Server::ResponseCallback& callback) {
            std::string target = request.target();
            if(auto pos = target.find('?'); pos != std::string::npos) {
                target.resize(pos);
            }
            try {
                TAU_LOG_INFO("method: " << request.method() << ", target: " << target);

                if((request.method() == beast_http::verb::get) && (target == "/")) {
                    beast_response response{beast_http::status::ok, request.version()};
                    response.set(beast_http::field::content_type, "text/html");

                    auto html_page = ReadFile(std::string{PROJECT_SOURCE_DIR} + "/data/html/webrtc-test.html");
                    beast::ostream(response.body()) << html_page;
                    response.content_length(response.body().size());

                    callback(std::move(response));
                } else if((request.method() == beast_http::verb::post) && (target == "/offer")) {
                    std::lock_guard lock{_mutex};
                    auto offer = beast::buffers_to_string(request.body().data());
                    auto answer = ProcessSdpOffer(offer);

                    beast_response response{beast_http::status::ok, request.version()};
                    beast::ostream(response.body()) << answer;
                    response.content_length(response.body().size());

                    callback(std::move(response));
                } else if((request.method() == beast_http::verb::post) && (target == "/candidate")) {
                    std::lock_guard lock{_mutex};
                    auto candidates = beast::buffers_to_string(request.body().data());
                    boost_ec ec;
                    auto parsed = Json::parse(candidates, ec);
                    if(!ec) {
                        constexpr std::string_view kCandidatePrefix = "candidate:";
                        if(parsed.is_array()) {
                            for(auto& element : parsed.get_array()) {
                                if(element.is_string()) {
                                    const auto candidate = boost::json::value_to<std::string>(element);
                                    if(IsPrefix(candidate, kCandidatePrefix)) {
                                        _pc->SetRemoteIceCandidate(candidate.substr(kCandidatePrefix.size()));
                                    }
                                } else {
                                    TAU_LOG_WARNING("Skipped element: " << element);
                                }
                            }
                        }
                    }
                    
                    beast_response response{beast_http::status::ok, request.version()};
                    auto local_ice_candidates = _pc->GetLocalCandidates();
                    if(!local_ice_candidates.empty()) {
                        beast::ostream(response.body()) << "[";
                        for(size_t i = 0; i < local_ice_candidates.size(); ++i) {
                            auto& local_candidate = local_ice_candidates.at(i);
                            if(i != 0) {
                                beast::ostream(response.body()) << ",";
                            }
                            beast::ostream(response.body()) << '"' << local_candidate << '"';
                        }
                        beast::ostream(response.body()) << "]";
                    }
                    response.content_length(response.body().size());

                    callback(std::move(response));
                } else {
                    beast_response response{beast_http::status::not_found, request.version()};
                    callback(std::move(response));
                }
            } catch(const std::exception& e) {
                TAU_LOG_WARNING("Exception: " << e.what());
                beast_response bad_request{beast::http::status::bad_request, request.version()};
                return callback(std::move(bad_request));
            }
        });
    server.Start();

    while(true) {
        std::this_thread::sleep_for(5ms);
        Process();
    }
    // Event().WaitFor(600s);
}

}
