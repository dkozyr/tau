#include "tau/rtp/TsConverter.h"

namespace rtp {

TsConverter::TsConverter(Options&& options)
    : _rate(options.rate)
    , _ts_last(options.ts_base)
    , _tp(options.tp_base)
{}

Timepoint TsConverter::FromTs(uint32_t ts) {
    const auto ts_delta = ts - _ts_last;
    if(ts_delta >= kTsNegativeThreshold) {
        const auto negative_ts_delta = _ts_last - ts;
        return _tp - kSec * negative_ts_delta / _rate;
    }
    _ts_last = ts;
    _tp += kSec * ts_delta / _rate;
    return _tp;
}

uint32_t TsConverter::FromTp(Timepoint tp) {
    const auto tp_delta = tp - _tp;
    const auto ts = _ts_last + static_cast<uint32_t>(tp_delta * _rate / kSec);
    if(tp_delta > kTpDeltaThreshold) {
        _tp = tp;
        _ts_last = ts;
    }
    return ts;
}

}
