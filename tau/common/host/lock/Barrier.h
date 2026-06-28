#pragma once

#include <atomic>

namespace tau::lock {

static inline void BarrierAcquire(void) {
    std::atomic_thread_fence(std::memory_order_acquire);
}

static inline void BarrierRelease(void) {
    std::atomic_thread_fence(std::memory_order_release);
}

}
