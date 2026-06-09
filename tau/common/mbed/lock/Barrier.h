#pragma once

namespace tau::lock {

static inline void BarrierAcquire(void) {
    __asm__ __volatile__("memw" ::: "memory");

    //TODO: consider this one:
    //portMEMORY_BARRIER();
}

static inline void BarrierRelease(void) {
    __asm__ __volatile__("memw" ::: "memory");

    //TODO: consider this one:
    //portMEMORY_BARRIER();
}

}
