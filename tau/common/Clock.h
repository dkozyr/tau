#pragma once

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <string>

using Timepoint = uint64_t; // nanoseconds

constexpr Timepoint kSec = 1'000'000'000;
constexpr Timepoint kMs = 1'000'000;
constexpr Timepoint kMicro = 1'000;

class Clock {
public:
    virtual ~Clock() = default;

    virtual Timepoint Now() const = 0;
};

class SteadyClock : public Clock {
public:
    Timepoint Now() const override {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(_clock.now().time_since_epoch()).count();
    }

private:
    std::chrono::steady_clock _clock;
};

inline double DurationSec(Timepoint a, Timepoint b) {
    return static_cast<double>(b - a) * 1e-9;
}

inline double DurationSec(Timepoint a) {
    return static_cast<double>(a) * 1e-9;
}
