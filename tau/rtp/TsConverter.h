#pragma once

#include "tau/common/Clock.h"
#include <limits>

namespace tau::rtp {

class TsConverter {
public:
    static constexpr uint32_t kTsNegativeThreshold = 0x8000'0000;
    static constexpr uint32_t kTpDeltaThreshold = std::numeric_limits<uint32_t>::max();

    struct Options {
        size_t rate;
        uint32_t ts_base;
        Timepoint tp_base = 0;
    };

public:
    TsConverter(Options&& options);

    Timepoint FromTs(uint32_t ts);
    uint32_t FromTp(Timepoint tp);

private:
    const uint32_t _rate;
    uint32_t _ts_last;
    Timepoint _tp;
};

}
