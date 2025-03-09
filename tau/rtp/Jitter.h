#pragma once

#include "tau/common/Clock.h"
#include <cassert>

namespace rtp {

// https://datatracker.ietf.org/doc/html/rfc3550#appendix-A.8
class Jitter {
public:
    Jitter(uint32_t clock_rate, uint32_t ts, Timepoint tp)
        : _clock_rate(clock_rate)
        , _ts(ts)
        , _tp(tp)
    {}

    uint32_t Get() const {
        return _jitter >> 4;
    }

    void Update(uint32_t ts, Timepoint tp) {
        assert(_tp <= tp && "Steady clock produces non-decreasing timepoints");
        const auto dts = Delta(_ts, ts);
        const auto dtp = (tp - _tp) * _clock_rate / kSec;
        const auto dt = Delta(dts, static_cast<uint32_t>(dtp));
        _jitter += dt - ((_jitter + 8) >> 4);
        _ts = ts;
        _tp = tp;
    }

private:
    static uint32_t Delta(uint32_t a, uint32_t b) {
        return (a < b) ? (b - a) : (a - b);
    }

private:
    const uint32_t _clock_rate;
    uint32_t _jitter = 0;
    uint32_t _ts;
    Timepoint _tp;
};

}
