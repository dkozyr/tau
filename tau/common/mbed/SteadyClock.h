#pragma once

#include <tau/common/Clock.h>
#include "esp_timer.h"

namespace tau {

class SteadyClock : public Clock {
public:
    Timepoint Now() const override {
        return static_cast<Timepoint>(esp_timer_get_time()) * kMicro;
    }
};

}
