#include "tau/rtcp/PacketLostWord.h"
#include "tau/common/Random.h"
#include "tau/common/Log.h"
#include <gtest/gtest.h>

namespace rtcp {

TEST(PacketLostWordTest, CumulativeLost) {
    const PacketLostWord packet_lost_word = 0x77123456;
    ASSERT_EQ(0x77, GetFractionLost(packet_lost_word));
    ASSERT_EQ(0x123456, GetCumulativePacketLost(packet_lost_word));
}

TEST(PacketLostWordTest, CumulativeLost_Negative) {
    const PacketLostWord packet_lost_word = 0x77FFFFFF;
    ASSERT_EQ(0x77, GetFractionLost(packet_lost_word));
    ASSERT_EQ(-1, GetCumulativePacketLost(packet_lost_word));
}

TEST(PacketLostWordTest, CumulativeLost_Zero) {
    const PacketLostWord packet_lost_word = 0x77000000;
    ASSERT_EQ(0x77, GetFractionLost(packet_lost_word));
    ASSERT_EQ(0, GetCumulativePacketLost(packet_lost_word));
}

TEST(PacketLostWordTest, CumulativeLost_Min) {
    const PacketLostWord packet_lost_word = 0x77800000;
    ASSERT_EQ(0x77, GetFractionLost(packet_lost_word));
    ASSERT_EQ(kCumulativeLostMin, GetCumulativePacketLost(packet_lost_word));
}

TEST(PacketLostWordTest, CumulativeLost_Max) {
    const PacketLostWord packet_lost_word = 0x777FFFFF;
    ASSERT_EQ(0x77, GetFractionLost(packet_lost_word));
    ASSERT_EQ(kCumulativeLostMax, GetCumulativePacketLost(packet_lost_word));
}

TEST(PacketLostWordTest, CumulativeLost_Randomized) {
    Random random;
    for(size_t i = 0; i < 50'000; ++i) {
        const auto fraction_lost = random.Int<uint8_t>();
        const auto cumulative_packet_lost = random.Int<int32_t>(kCumulativeLostMin, kCumulativeLostMax);
        const PacketLostWord packet_lost_word = BuildPacketLostWord(fraction_lost, cumulative_packet_lost);
        ASSERT_EQ(fraction_lost, GetFractionLost(packet_lost_word));
        ASSERT_EQ(cumulative_packet_lost, GetCumulativePacketLost(packet_lost_word));
    }
}

TEST(PacketLostWordTest, DISABLED_CumulativeLost_AllPositiveNumbers) {
    for(int32_t i = 0; i <= kCumulativeLostMax; ++i) {
        auto word = BuildPacketLostWord(0x77, i);
        ASSERT_EQ(i, GetCumulativePacketLost(word));
    }
}

TEST(PacketLostWordTest, DISABLED_CumulativeLost_AllNegativeNumbers) {
    for(int32_t i = kCumulativeLostMin; i < 0; ++i) {
        auto word = BuildPacketLostWord(0x77, i);
        ASSERT_EQ(i, GetCumulativePacketLost(word));
    }
}

}
