#include "tests/webrtc/PeerConnectionBaseTest.h"

namespace tau::webrtc {

struct PeerConnectionDirectionTestParams {
    sdp::Direction audio1;
    sdp::Direction video1;

    sdp::Direction audio2;
    sdp::Direction video2;
};

std::vector<PeerConnectionDirectionTestParams> MakePeerConnectionDirectionTestParams() {
    const std::vector<sdp::Direction> kRecvDirections = {
        sdp::Direction::kInactive,
        sdp::Direction::kRecv,
    };
    const std::vector<sdp::Direction> kSendDirections = {
        sdp::Direction::kInactive,
        sdp::Direction::kSend,
    };

    std::vector<PeerConnectionDirectionTestParams> params;
    for(auto audio1 : kRecvDirections) {
    for(auto video1 : kRecvDirections) {
        for(auto audio2 : kSendDirections) {
        for(auto video2 : kSendDirections) {
            params.push_back(PeerConnectionDirectionTestParams{
                .audio1 = static_cast<sdp::Direction>(audio1 | kSendDirections[g_random.Int(0, 1)]),
                .video1 = static_cast<sdp::Direction>(video1 | kSendDirections[g_random.Int(0, 1)]),
                .audio2 = static_cast<sdp::Direction>(audio2 | kRecvDirections[g_random.Int(0, 1)]),
                .video2 = static_cast<sdp::Direction>(video2 | kRecvDirections[g_random.Int(0, 1)]),
            });
        }}
    }}
    return params;
}

class PeerConnectionDirectionTest
    : public PeerConnectionBaseTest
    , public ::testing::TestWithParam<PeerConnectionDirectionTestParams>
{};

INSTANTIATE_TEST_SUITE_P(P, PeerConnectionDirectionTest, ::testing::ValuesIn(MakePeerConnectionDirectionTestParams()));

TEST_P(PeerConnectionDirectionTest, Test) {
    auto params = GetParam();
    CallContext ctx(
        CreatePcDependencies(),
        CallContext::Options{
            .offerer = ClientContext::Options{
                .audio = params.audio1,
                .video = params.video1,
                .log_ctx = "[offerer] "
            },
            .answerer = ClientContext::Options{
                .audio = params.audio2,
                .video = params.video2,
                .log_ctx = "[answerer] "
            },
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
