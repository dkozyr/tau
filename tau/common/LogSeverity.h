#pragma once

#include <cstdint>

namespace tau {

enum Severity : uint32_t {
    kTrace   = 0,
    kDebug   = 1,
    kInfo    = 2,
    kWarning = 3,
    kError   = 4,
    kFatal   = 5
};

//TODO: get from CMake
inline constexpr Severity kSeverityDefault = Severity::kInfo;

}
