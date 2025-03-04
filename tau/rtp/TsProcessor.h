#pragma once

#include "tau/common/Clock.h"

namespace rtp {

// Converting RTP timestamp to Timepoint starting from point 0.0 sec
class TsProcessor {
public:
    static constexpr uint32_t kTsNegativeThreshold = 0x8000'0000;

    struct Options {
        uint32_t rate;
        uint32_t ts_base;
    };

public:
    TsProcessor(Options&& options)
        : _rate(options.rate)
        , _ts_last(options.ts_base)
    {}

    Timepoint Process(uint32_t ts) {
        const auto ts_delta = ts - _ts_last;
        if(ts_delta >= kTsNegativeThreshold) {
            const auto negative_ts_delta = _ts_last - ts;
            return _tp - kSec * negative_ts_delta / _rate;
        }
        _ts_last = ts;
        _tp += kSec * ts_delta / _rate;
        return _tp;
    }

private:
    const uint32_t _rate;
    uint32_t _ts_last;
    Timepoint _tp = 0;
};

}
