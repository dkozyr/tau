#include "tests/webrtc/PeerConnectionBaseTest.h"

namespace tau::webrtc {

class PeerConnectionLossTest : public PeerConnectionBaseTest , public ::testing::Test {};

TEST_F(PeerConnectionLossTest, Test) {
    CallContext ctx(
        CreatePcDependencies(),
        CallContext::Options{
            .offerer = ClientContext::Options{
                .loss_rate = 0.1,
                .log_ctx = "[offerer] "
            },
            .answerer = ClientContext::Options{
                .loss_rate = 0.1,
                .log_ctx = "[answerer] "
            },
        });
    ASSERT_NO_FATAL_FAILURE(ctx.SdpNegotiation());
    ASSERT_NO_FATAL_FAILURE(ctx.ProcessLocalCandidates());
    ASSERT_NO_FATAL_FAILURE(ctx.ProcessUntilState(State::kConnected));

    for(size_t i = 0; i < 10; ++i) {
        std::this_thread::sleep_for(33ms);
        if(i % 2 == 0) {
            ctx._pc1.PushFrame(kAudioMediaIdx);
            ctx._pc2.PushFrame(kAudioMediaIdx);
        }
        ctx._pc1.PushFrame(kVideoMediaIdx);
        ctx._pc2.PushFrame(kVideoMediaIdx);
    }
    EXPECT_NO_FATAL_FAILURE(ctx.ProcessUntil([&]() {
        if(ctx._pc2._send_packets[kVideoMediaIdx].size() != ctx._pc1._recv_packets[kVideoMediaIdx].size()) {
            return false;
        }
        if(ctx._pc1._send_packets[kVideoMediaIdx].size() != ctx._pc2._recv_packets[kVideoMediaIdx].size()) {
            return false;
        }
        return true;
    }));

    ctx.Stop();
}

}
