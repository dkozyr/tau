#pragma once

#include <cstdint>

namespace rtcp {

#pragma pack(push,1)

//https://datatracker.ietf.org/doc/html/rfc3550#section-6.4.1
struct SrInfo {
    uint64_t ntp;
    uint32_t ts;
    uint32_t packet_count;
    uint32_t octet_count;
};

#pragma pack(pop)

inline bool operator==(const SrInfo& a, const SrInfo& b) {
    return (a.ntp == b.ntp) && (a.ts == b.ts) && (a.packet_count == b.packet_count) && (a.octet_count == b.octet_count);
}

}
