#pragma once

#include <cstddef>

#ifdef ESP_PLATFORM
    inline constexpr size_t kCacheLine = 32;
#else
    inline constexpr size_t kCacheLine = 64;
#endif
