#include "tests/webrtc/PeerConnectionPairContext.h"
#include "tau/webrtc/PeerConnection.h"
#include "tau/http/Server.h"
#include "tau/http/Client.h"
#include "tau/rtp-session/FrameProcessor.h"
#include "tau/rtp-packetization/H264Depacketizer.h"
#include "tau/rtp-packetization/H264Packetizer.h"
#include "tau/rtp/RtpAllocator.h"
#include "tau/video/h264/Avc1NaluProcessor.h"
#include "tau/video/h264/AnnexB.h"
#include "tau/asio/ThreadPool.h"
#include "tau/common/File.h"
#include "tau/common/Json.h"
#include "tau/common/File.h"
#include "tau/common/Ntp.h"
#include "tests/lib/Common.h"

namespace tau::webrtc {

class PeerConnectionTest public ::testing::Test {
public:
    PeerConnectionTest()
        : _io(std::thread::hardware_concurrency())
    {}

    ~PeerConnectionTest() {
        _io.Join();
    }

protected:
    PeerConnection::Options::Sdp CreateSdpOptions() {
        return PeerConnection::Options::Sdp{
            .audio = sdp::Media{
                .type = sdp::MediaType::kAudio,
                .mid = {},
                .direction = sdp::Direction::kInactive,
                .codecs = {
                    { 8, sdp::Codec{.index = 0, .name = "PCMU", .clock_rate = 8000}},
                },
                .ssrc = std::nullopt
            },
            .video = sdp::Media{
                .type = sdp::MediaType::kVideo,
                .mid = {},
                .direction = sdp::Direction::kSendRecv,
                .codecs = {
                    {100, sdp::Codec{.index = 0, .name = "H264", .clock_rate = 90000, .rtcp_fb = sdp::kRtcpFbDefault,
                        .format = "level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=620028"}},
                    {101, sdp::Codec{.index = 1, .name = "H264", .clock_rate = 90000, .rtcp_fb = sdp::kRtcpFbDefault,
                        .format = "level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=4d0028"}},
                    {102, sdp::Codec{.index = 2, .name = "H264", .clock_rate = 90000, .rtcp_fb = sdp::kRtcpFbDefault,
                        .format = "level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=420028"}},
                },
                .ssrc = std::nullopt
            }
        };
    }

    PeerConnectionContext::Dependencies CreatePcDependencies() {
        return PeerConnectionContext::Dependencies{
            .clock = _clock,
            .executor = _io.GetStrand(),
            .udp_allocator = g_udp_allocator
        };
    }

protected:
    ThreadPool _io;
    SteadyClock _clock;
    std::mutex _mutex;
    std::optional<PeerConnection> _pc;

    struct PcContext {
        PcContext(uint8_t pt, uint32_t ssrc, uint32_t clock_rate, std::string output_filename)
            : base_tp(g_random.Int<uint32_t>())
            , rtp_allocator_video(g_udp_allocator,
                    rtp::RtpAllocator::Options{
                    .header = rtp::Writer::Options{
                        .pt = pt,
                        .ssrc = ssrc,
                        .ts = base_tp,
                        .sn = g_random.Int<uint16_t>(),
                        .marker = false
                    },
                    .base_tp = base_tp,
                    .clock_rate = clock_rate
                })
            , h264_packetizer(rtp_allocator_video)
            , h264_depacketizer(g_system_allocator)
            , frame_processor()
            , avc1_nalu_processor(h264::Avc1NaluProcessor::Options{})
            , output_path(output_filename)
        {}

        const uint32_t base_tp;
        rtp::RtpAllocator rtp_allocator_video;
        rtp::H264Packetizer h264_packetizer;
        rtp::H264Depacketizer h264_depacketizer;
        rtp::session::FrameProcessor frame_processor;
        h264::Avc1NaluProcessor avc1_nalu_processor;
        std::filesystem::path output_path;
    };
    std::optional<PcContext> _ctx;
};

TEST_F(PeerConnectionTest, Basic) {
    PeerConnectionPairContext ctx(
        CreatePcDependencies(),
        PeerConnectionPairContext::Options{
            .offerer = PeerConnectionContext::Options{.log_ctx = "[offerer] "},
            .answerer = PeerConnectionContext::Options{.log_ctx = "[answerer] "},
        });
    ASSERT_NO_FATAL_FAILURE(ctx.SdpNegotiation());
    ASSERT_NO_FATAL_FAILURE(ctx.ProcessLocalCandidates());
    ASSERT_NO_FATAL_FAILURE(ctx.ProcessUntilState(State::kConnected));

    for(size_t i = 0; i < 10; ++i) {
        std::this_thread::sleep_for(1ms);
        if(i % 2 == 0) {
            ctx._pc1.PushFrame(kAudioMediaIdx);
            ctx._pc2.PushFrame(kAudioMediaIdx);
        }
        ctx._pc1.PushFrame(kVideoMediaIdx);
        ctx._pc2.PushFrame(kVideoMediaIdx);
    }
    EXPECT_NO_FATAL_FAILURE(ctx.ProcessUntilDone());
    ctx.Stop();
}

}
