#pragma once

#include <cstdint>
#include <set>

namespace tau::rtcp {

// https://datatracker.ietf.org/doc/html/rfc4585#section-6.2.1
// The Generic NACK message is identified by PT=RTPFB and FMT=1

#pragma pack(push,1)

struct NackMessage {
    uint16_t pid;
    uint16_t blp;
};

#pragma pack(pop)

using NackSns = std::set<uint16_t>;

}
