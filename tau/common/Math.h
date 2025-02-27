#pragma once

#include <cmath>
#include <algorithm>
#include <limits>
#include <utility>
#include <cstddef>

inline constexpr size_t Align(size_t value, size_t alignment) {
    return alignment * ((value + alignment - 1) / alignment);
}

inline constexpr size_t DivCeil(size_t a, size_t b) {
    return (a + b - 1) / b;
}

template<typename T>
bool Near(T a, T b) {
    return std::abs(a - b) < std::numeric_limits<T>::epsilon();
}
