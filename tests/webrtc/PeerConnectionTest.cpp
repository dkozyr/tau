#include "tests/webrtc/PeerConnectionBaseTest.h"

namespace tau::webrtc {

class PeerConnectionTest : public PeerConnectionBaseTest, public ::testing::Test {};

TEST_F(PeerConnectionTest, Basic) {
    CallContext ctx(
        CreatePcDependencies(),
        CallContext::Options{
            .offerer = ClientContext::Options{.log_ctx = "[offerer] "},
            .answerer = ClientContext::Options{.log_ctx = "[answerer] "},
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
