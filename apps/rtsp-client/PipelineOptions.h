#pragma once

#include "tau/memory/Buffer.h"
#include <optional>

namespace tau::rtsp {

struct PipelineOptions {
    uint32_t clock_rate = 90000;
    std::optional<Buffer> sps = std::nullopt;
    std::optional<Buffer> pps = std::nullopt;
};

}
