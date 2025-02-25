#pragma once

#include <cmath>
#include <algorithm>
#include <limits>
#include <utility>

template<typename T>
bool Near(T a, T b) {
    return std::abs(a - b) < std::numeric_limits<T>::epsilon();
}
