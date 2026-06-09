#pragma once

#include <cstddef>

#ifdef ESP_PLATFORM
    #include <tau/memory/mbed/PoolAllocator.h>
#else
    #include <tau/memory/host/PoolAllocator.h>
#endif

namespace tau {

inline constexpr size_t kUdpMtuSize = 1500;

}
