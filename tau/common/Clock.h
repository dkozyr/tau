#pragma once

#include <chrono>
#include <cstdint>
#include <string>

namespace tau {

using Timepoint = uint64_t; // nanoseconds

constexpr Timepoint kMicro = 1'000;
constexpr Timepoint kMs    = 1'000'000;
constexpr Timepoint kSec   = 1'000'000'000;
constexpr Timepoint kMin   = 60 * kSec;
constexpr Timepoint kHour  = 60 * kMin;

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

double DurationSec(Timepoint a, Timepoint b);
double DurationSec(Timepoint a);

double DurationMs(Timepoint a, Timepoint b);
double DurationMs(Timepoint a);

int64_t DurationMsInt(Timepoint a, Timepoint b);
int64_t DurationMsInt(Timepoint a);

Timepoint FromIso8601(const std::string& iso);

}
