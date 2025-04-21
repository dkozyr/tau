#include "tau/webrtc/PeerConnection.h"
#include "tau/http/Server.h"
#include "tau/http/Client.h"
#include "tau/asio/ThreadPool.h"
#include "tau/common/File.h"
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
        _pc.emplace(
            PeerConnection::Dependencies{
                .clock = _clock,
                .executor = _io.GetExecutor(),
                .udp_allocator = g_udp_allocator
            }
        );
        return _pc->ProcessSdpOffer(offer);
    }

protected:
    ThreadPool _io;
    SteadyClock _clock;
    std::optional<PeerConnection> _pc;
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
                    auto offer = beast::buffers_to_string(request.body().data());
                    auto answer = ProcessSdpOffer(offer);

                    beast_response response{beast_http::status::ok, request.version()};
                    beast::ostream(response.body()) << answer;
                    response.content_length(response.body().size());

                    callback(std::move(response));
                } else if((request.method() == beast_http::verb::post) && (target == "/candidate")) {
                    auto candidate = beast::buffers_to_string(request.body().data());
                    TAU_LOG_INFO("candidate: " << candidate);
                    
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
        if(_pc) {
            _pc->Process();
        }
    }
    // Event().WaitFor(600s);
}

}
