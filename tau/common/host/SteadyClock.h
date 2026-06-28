#pragma once

#include <tau/common/Clock.h>
#include <chrono>

namespace tau {

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

}
