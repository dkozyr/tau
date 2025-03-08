#pragma once

#include "tau/rtcp/PacketLostWord.h"
#include <vector>

namespace rtcp {

#pragma pack(push,1)

//https://datatracker.ietf.org/doc/html/rfc3550#section-6.4.1
struct RrBlock {
    uint32_t ssrc;
    PacketLostWord packet_lost_word;
    uint32_t ext_highest_sn;
    uint32_t jitter;
    uint32_t lsr;
    uint32_t dlsr;
};

#pragma pack(pop)

inline bool operator==(const RrBlock& a, const RrBlock& b) {
    return (a.ssrc == b.ssrc) && (a.packet_lost_word == b.packet_lost_word)
        && (a.ext_highest_sn == b.ext_highest_sn) && (a.jitter == b.jitter) && (a.lsr == b.lsr) && (a.dlsr == b.dlsr);
}

using RrBlocks = std::vector<RrBlock>;

}
