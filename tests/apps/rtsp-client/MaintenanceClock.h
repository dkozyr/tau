#pragma once

#include "tau/common/Clock.h"
#include <atomic>

namespace tau::rtsp {

class MaintenanceClock : public Clock {
public:
    Timepoint Now() const override {
        return _now.load();
    }

    void Add(Timepoint duration) {
        _now.fetch_add(duration);
    }

private:
    std::atomic<Timepoint> _now{0};
};

}
