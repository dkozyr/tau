#pragma once

#include <cstdint>
#include <cstddef>

namespace tau::sdp {

enum Direction : uint8_t {
    kInactive = 0,
    kSend     = 1,
    kRecv     = 2,
    kSendRecv = (1 | 2),
};

}
