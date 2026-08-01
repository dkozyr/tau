#pragma once

#include "tau/common/SystemClock.h"
#include "tau/common/Log.h"
#include <thread>

namespace tau {

template<typename TFunction>
bool WaitForCondition(TFunction&& condition, Timepoint timeout = 1 * kSec) {
    SystemClock clock;
    auto start_time = clock.Now();
    while(!condition()) {
        if(clock.Now() - start_time > timeout) {
            TAU_LOG_ERROR("Error: timeout");
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return true;
}

}
