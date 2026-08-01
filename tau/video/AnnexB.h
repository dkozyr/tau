#pragma once

#include <etl/array.h>
#include <cstdint>
#include <cstddef>

namespace tau {

constexpr etl::array<uint8_t, 4> kAnnexB = {0, 0, 0, 1};
constexpr etl::array<uint8_t, 3> kAnnexBShort = {0, 0, 1};

}
