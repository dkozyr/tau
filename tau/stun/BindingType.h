#pragma once

#include <cstdint>

namespace tau::stun {

constexpr uint16_t kBindingMethod = 0x0001;

enum BindingType : uint16_t {
    kRequest       = 0x0000 | kBindingMethod,
    kIndication    = 0x0010 | kBindingMethod,
    kResponse      = 0x0100 | kBindingMethod,
    kErrorResponse = 0x0110 | kBindingMethod
};

}
