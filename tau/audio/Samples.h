#pragma once

#include "tau/common/Clock.h"

namespace tau::audio {

inline constexpr uint32_t kClockRate = 48000; // Opus uses fixed rtp clock rate

inline Timepoint SamplesToDuration(size_t samples_per_channel, size_t sample_rate = kClockRate) {
    return kSec * samples_per_channel / sample_rate;
}

inline Timepoint BytesToSamples(size_t bytes, size_t channels) {
    return bytes / sizeof(int16_t) / channels;
}

}
