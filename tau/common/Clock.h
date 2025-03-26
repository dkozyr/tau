#pragma once

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <string>

namespace tau {

using Timepoint = uint64_t; // nanoseconds

constexpr Timepoint kMicro = 1'000;
constexpr Timepoint kMs    = 1'000'000;
constexpr Timepoint kSec   = 1'000'000'000;
constexpr Timepoint kMin   = 60 * kSec;

class Clock {
public:
    virtual ~Clock() = default;

    virtual Timepoint Now() const = 0;
};

template<typename T>
class TClock : public Clock {
public:
    Timepoint Now() const override {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(_clock.now().time_since_epoch()).count();
    }

private:
    T _clock;
};
using SteadyClock = TClock<std::chrono::steady_clock>;
using SystemClock = TClock<std::chrono::system_clock>;

inline double DurationSec(Timepoint a, Timepoint b) {
    return static_cast<double>(b - a) * 1e-9;
}

inline double DurationSec(Timepoint a) {
    return static_cast<double>(a) * 1e-9;
}

inline double DurationMs(Timepoint a, Timepoint b) {
    return (b - a) / kMs;
}

inline double DurationMs(Timepoint a) {
    return a / kMs;
}

}
