#include "tau/rtcp/PacketLostWord.h"
#include "tests/Common.h"

namespace tau::rtcp {

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

TEST(PacketLostWordTest, Build_ZeroLoss) {
    const uint32_t received = 123;
    const uint32_t expected = 123;
    const auto fraction_lost = BuildFractionLost(received, expected);
    const auto cumulative_packet_lost = BuildCumulativePacketLost(received, expected);
    const auto packet_lost_word = BuildPacketLostWord(fraction_lost, cumulative_packet_lost);
    ASSERT_EQ(0, GetFractionLost(packet_lost_word));
    ASSERT_EQ(0, GetCumulativePacketLost(packet_lost_word));
}

TEST(PacketLostWordTest, Build_PositiveLoss) {
    const uint32_t received = 123;
    const uint32_t expected = 150;
    const auto fraction_lost = BuildFractionLost(received, expected);
    const auto cumulative_packet_lost = BuildCumulativePacketLost(received, expected);
    const auto packet_lost_word = BuildPacketLostWord(fraction_lost, cumulative_packet_lost);
    constexpr auto kTargetFractionLost = (150 - 123) * 256 / 150;
    ASSERT_EQ(kTargetFractionLost, GetFractionLost(packet_lost_word));
    ASSERT_EQ(150 - 123, GetCumulativePacketLost(packet_lost_word));
}

TEST(PacketLostWordTest, Build_NegativeLoss) {
    const uint32_t received = 150;
    const uint32_t expected = 123;
    const auto fraction_lost = BuildFractionLost(received, expected);
    const auto cumulative_packet_lost = BuildCumulativePacketLost(received, expected);
    const auto packet_lost_word = BuildPacketLostWord(fraction_lost, cumulative_packet_lost);
    ASSERT_EQ(0, GetFractionLost(packet_lost_word));
    ASSERT_EQ(-27, GetCumulativePacketLost(packet_lost_word));
}

TEST(PacketLostWordTest, Build_ZeroExpected) {
    const uint32_t received = 123;
    const uint32_t expected = 0;
    const auto fraction_lost = BuildFractionLost(received, expected);
    const auto cumulative_packet_lost = BuildCumulativePacketLost(received, expected);
    const auto packet_lost_word = BuildPacketLostWord(fraction_lost, cumulative_packet_lost);
    ASSERT_EQ(0, GetFractionLost(packet_lost_word));
    ASSERT_EQ(-123, GetCumulativePacketLost(packet_lost_word));
}

TEST(PacketLostWordTest, Build_HugeFractionLost) {
    const uint32_t received = 1;
    const uint32_t expected = 123;
    const auto fraction_lost = BuildFractionLost(received, expected);
    const auto cumulative_packet_lost = BuildCumulativePacketLost(received, expected);
    const auto packet_lost_word = BuildPacketLostWord(fraction_lost, cumulative_packet_lost);
    constexpr auto kTargetFractionLost = (123 - 1) * 256 / 123;
    ASSERT_EQ(kTargetFractionLost, GetFractionLost(packet_lost_word));
    ASSERT_EQ(122, GetCumulativePacketLost(packet_lost_word));
}

TEST(PacketLostWordTest, Build_ZeroReceived_ImpossibleCase_WeHaveToReceiveAtLeastOnePacketToEstimateExpected) {
    const uint32_t received = 0;
    const uint32_t expected = 123;
    const auto packet_lost_word = BuildPacketLostWord(received, expected);
    ASSERT_EQ(0, GetFractionLost(packet_lost_word));
    ASSERT_EQ(123, GetCumulativePacketLost(packet_lost_word));
}

TEST(PacketLostWordTest, CumulativeLost_Randomized) {
    for(size_t i = 0; i < 50'000; ++i) {
        const auto fraction_lost = g_random.Int<uint8_t>();
        const auto cumulative_packet_lost = g_random.Int<int32_t>(kCumulativeLostMin, kCumulativeLostMax);
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
