#pragma once

#include <random>

class Random {
public:
    Random() : _gen(_rd()) {}

    template<typename T = double>
    T Real(T a = 0.0, T b = 1.0) {
        std::uniform_real_distribution<T> uniform(a, b);
        return uniform(_gen);
    }

    template<typename T = int>
    T Int(T a, T b) {
        std::uniform_int_distribution<T> uniform(a, b);
        return uniform(_gen);
    }

    template<typename T = int>
    T Int() {
        return Int(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
    }

private:
    std::random_device _rd;
    std::mt19937 _gen;
};
