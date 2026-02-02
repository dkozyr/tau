#pragma once

#include <cmath>
#include <algorithm>
#include <limits>
#include <utility>
#include <type_traits>
#include <cstddef>

namespace tau {

inline constexpr size_t Align(size_t value, size_t alignment) {
    return alignment * ((value + alignment - 1) / alignment);
}

inline constexpr size_t DivCeil(size_t a, size_t b) {
    return (a + b - 1) / b;
}

template<typename T>
inline T AbsDelta(T a, T b) {
    return (a < b) ? (b - a) : (a - b);
}

template<typename T>
bool Near(T a, T b) {
    return std::abs(a - b) < std::numeric_limits<T>::epsilon();
}

template<typename T>
constexpr T AlignPowerOfTwo(T x) {
    static_assert(std::is_integral_v<T> && std::is_unsigned_v<T>);

    if(x == 0) { return 1 ;}
    x--;
    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);
    x |= (x >> 8);
    x |= (x >> 16);
    if constexpr(sizeof(T) > 4) {
        x |= (x >> 32);
    }
    return x + 1;
}

}
