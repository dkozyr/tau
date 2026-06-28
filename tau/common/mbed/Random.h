#pragma once

#include "esp_random.h"
#include <limits>

namespace tau {

class Random {
public:
    Random() = default;

    template<typename T = double>
    T Real(T a = 0.0, T b = 1.0) {
        const T rand = esp_random() / static_cast<T>(std::numeric_limits<uint32_t>::max());
        return static_cast<T>(a + rand * (b - a));
    }

    template<typename T = int>
    T Int(T a, T b) {
        if constexpr(sizeof(T) <= sizeof(uint32_t)) {
            return static_cast<T>(a + esp_random() % (b - a));
        } else {
            const uint64_t high = esp_random();
            const uint64_t low = esp_random();
            const uint64_t value = (high << 32) | low;
            return static_cast<T>(a + value % (b - a));
        }
    }

    template<typename T = int>
    T Int() {
        return Int(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
    }

    bool Bool() {
        return Int(0, 1) == 1;
    }
};

}
