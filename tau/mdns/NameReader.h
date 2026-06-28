#pragma once

#include <tau/mdns/Name.h>
#include <cstdint>

namespace tau::mdns {

Name ParseName(const uint8_t*& ptr, const uint8_t* end);

}
