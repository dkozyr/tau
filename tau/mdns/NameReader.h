#pragma once

#include <string>
#include <cstdint>

namespace tau::mdns {

std::string ParseName(const uint8_t*& ptr, const uint8_t* end);

}
