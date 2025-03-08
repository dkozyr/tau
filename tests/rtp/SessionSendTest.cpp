#include "tests/rtp/SessionBaseTest.h"

namespace rtp {

class SessionSendTest : public SessionBaseTest, public ::testing::Test {
public:
};

TEST_F(SessionSendTest, Basic) {
    constexpr auto kTestFrames = 30;
    constexpr auto kPacketPerFrame = 5;
    for(size_t i = 0; i < kTestFrames; ++i) {
        _media_clock.Add(33 * kMs);
        _source->PushFrame(_media_clock.Now(), kPacketPerFrame);
    }
    ASSERT_EQ(kTestFrames * kPacketPerFrame, _output_rtp.size());
    ASSERT_EQ(0, _output_rtcp.size());

    _media_clock.Add(33 * kMs);
    _source->PushFrame(_media_clock.Now(), kPacketPerFrame);
    ASSERT_EQ((kTestFrames + 1) * kPacketPerFrame, _output_rtp.size());
    ASSERT_EQ(1, _output_rtcp.size());
}

}
