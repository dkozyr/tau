#pragma once

#include "tau/rtcp/PacketLostWord.h"
#include <vector>

namespace tau::rtcp {

#pragma pack(push,1)

//https://datatracker.ietf.org/doc/html/rfc3550#section-6.4.1
struct RrBlock {
    uint32_t ssrc = 0;
    PacketLostWord packet_lost_word = 0;
    uint32_t ext_highest_sn = 0;
    uint32_t jitter = 0;
    uint32_t lsr = 0;
    uint32_t dlsr = 0;
};

#pragma pack(pop)

inline bool operator==(const RrBlock& a, const RrBlock& b) {
    return (a.ssrc == b.ssrc) && (a.packet_lost_word == b.packet_lost_word)
        && (a.ext_highest_sn == b.ext_highest_sn) && (a.jitter == b.jitter) && (a.lsr == b.lsr) && (a.dlsr == b.dlsr);
}

using RrBlocks = std::vector<RrBlock>;

}
