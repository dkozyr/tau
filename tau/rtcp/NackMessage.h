#pragma once

#include <etl/set.h>
#include <cstdint>

namespace tau::rtcp {

// https://datatracker.ietf.org/doc/html/rfc4585#section-6.2.1
// The Generic NACK message is identified by PT=RTPFB and FMT=1

#pragma pack(push,1)

struct NackMessage {
    uint16_t pid;
    uint16_t blp;
};

#pragma pack(pop)

using NackSns = etl::set<uint16_t, 32>;

}
