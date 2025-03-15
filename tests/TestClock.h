#pragma once

#include "tau/common/Clock.h"

namespace tau {

class TestClock : public Clock {
public:
    TestClock()
        : _now(SteadyClock().Now())
    {}

    Timepoint Now() const override {
        return _now;
    }

    void Add(uint64_t period) {
        _now += period;
    }

private:
    Timepoint _now;
};

}
