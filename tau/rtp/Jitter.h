#pragma once

#include "tau/common/Clock.h"
#include "tau/common/Math.h"
#include <cassert>

namespace rtp {

// https://datatracker.ietf.org/doc/html/rfc3550#appendix-A.8
class Jitter {
public:
    static constexpr uint32_t kTsBigJump = 0x4000'0000;

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
        const auto dts = AbsDelta(_ts, ts);
        if(dts < kTsBigJump) {
            const auto dtp = (tp - _tp) * _clock_rate / kSec;
            const auto dt = AbsDelta(dts, static_cast<uint32_t>(dtp));
            _jitter += dt - ((_jitter + 8) >> 4);
        }
        _ts = ts;
        _tp = tp;
    }

private:
    const uint32_t _clock_rate;
    uint32_t _jitter = 0;
    uint32_t _ts;
    Timepoint _tp;
};

}
