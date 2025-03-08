#pragma once

#include "tau/common/Clock.h"

using NtpTimepoint = uint64_t;

inline constexpr uint64_t kNtpSec = 0x1'0000'0000;
inline constexpr uint64_t kUnixTimeToNtpTimeSeconds = 2'208'988'800;

inline NtpTimepoint ToNtp(Timepoint system_clock_tp) {
    const auto div = std::lldiv(system_clock_tp, kSec);
    return (((div.quot + kUnixTimeToNtpTimeSeconds) << 32) | ((div.rem << 32) / kSec));
}

inline Timepoint FromNtp(NtpTimepoint ntp) {
    const auto seconds = (ntp >> 32);
    const auto fractions = (ntp & 0xFFFF'FFFF);
    constexpr uint64_t kHalfNanoRounding = 0x7FFF'FFFF;
    return (seconds - kUnixTimeToNtpTimeSeconds) * kSec + ((fractions * kSec + kHalfNanoRounding) >> 32);
}

inline uint32_t NtpToNtp32(NtpTimepoint ntp) {
    return static_cast<uint32_t>((ntp >> 16) & 0xFFFF'FFFF);
}
