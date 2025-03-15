#pragma once

#include <cstdint>
#include <algorithm>

namespace tau::rtcp {

// https://datatracker.ietf.org/doc/html/rfc3550#section-6.4.1
// | fraction lost |       cumulative number of packets lost       |
using PacketLostWord = uint32_t; 

inline constexpr int32_t kCumulativeLostMin = -0x80'0000;
inline constexpr int32_t kCumulativeLostMax = +0x7F'FFFF;

inline uint8_t GetFractionLost(PacketLostWord data) {
    return ((data >> 24) & 0xFF);
}

inline int32_t GetCumulativePacketLost(PacketLostWord data) {
    auto lost = (data & 0xFF'FFFF);
    constexpr uint32_t kSignBit = 0x80'0000;
    if(lost & kSignBit) {
        return -static_cast<int32_t>((lost ^ 0xFF'FFFF) + 1);
    } else {
        return static_cast<int32_t>(lost);
    }
}

inline PacketLostWord BuildPacketLostWord(uint8_t fraction_lost, int32_t cumulative_packet_lost) {
    PacketLostWord word = static_cast<uint32_t>(fraction_lost) << 24;
    cumulative_packet_lost = std::clamp<int32_t>(cumulative_packet_lost, kCumulativeLostMin, kCumulativeLostMax);
    if(cumulative_packet_lost < 0) {
        auto lost = static_cast<uint32_t>(-cumulative_packet_lost);
        return word | ((lost - 1) ^ 0xFF'FFFF);
    } else {
        return word | static_cast<uint32_t>(cumulative_packet_lost);
    }
}

inline PacketLostWord BuildPacketLostWord(uint32_t received_packets, uint32_t expected_packets) {
    const auto lost_packets = static_cast<int32_t>(expected_packets) - static_cast<int32_t>(received_packets);

    uint8_t fraction_lost = 0;
    if(lost_packets > 0) {
        const auto lost_packets_mul_256 = static_cast<uint32_t>(lost_packets) << 8;
        fraction_lost = static_cast<uint8_t>(lost_packets_mul_256 / expected_packets);
    }

    return BuildPacketLostWord(fraction_lost, lost_packets);
}

}
