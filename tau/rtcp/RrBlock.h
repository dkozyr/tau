#pragma once

#include <cstdint>
#include <vector>

namespace rtcp {

#pragma pack(push,1)

//https://datatracker.ietf.org/doc/html/rfc3550#section-6.4.1
struct RrBlock {
    uint32_t ssrc;
    uint8_t fraction_lost;
    uint8_t cumulative_lost[3]; //TODO: refactor it
    uint32_t ext_highest_sn;
    uint32_t jitter;
    uint32_t lsr;
    uint32_t dlsr;
};

#pragma pack(pop)

inline bool operator==(const RrBlock& a, const RrBlock& b) {
    return (a.ssrc == b.ssrc) && (a.fraction_lost == b.fraction_lost) && (a.cumulative_lost[0] == b.cumulative_lost[0])
        && (a.cumulative_lost[1] == b.cumulative_lost[1]) && (a.cumulative_lost[2] == b.cumulative_lost[2])
        && (a.ext_highest_sn == b.ext_highest_sn) && (a.jitter == b.jitter) && (a.lsr == b.lsr) && (a.dlsr == b.dlsr);
}

using RrBlocks = std::vector<RrBlock>;

}
