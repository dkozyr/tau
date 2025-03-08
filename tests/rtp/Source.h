#pragma once

#include "tau/rtp/RtpAllocator.h"
#include "tests/Common.h"

namespace rtp {

class Source {
public:
    struct Options{
        uint8_t pt;
        uint32_t ssrc;
        uint32_t clock_rate;
        uint32_t base_ts;
        Timepoint base_tp;
        uint16_t sn;
        uint16_t extension_length_in_words = 0;
        size_t max_packet_size = 1200;
    };

    using Callback = std::function<void(Buffer&&)>;

public:
    Source(Options&& options)
        : _options(std::move(options))
        , _allocator(RtpAllocator::Options{
            .header = Writer::Options{
                .pt = _options.pt,
                .ssrc = _options.ssrc,
                .ts = _options.base_ts,
                .sn = _options.sn,
                .marker = false,
                .extension_length_in_words = 0
            },
            .base_tp = _options.base_tp,
            .clock_rate = _options.clock_rate,
            .size = _options.max_packet_size
        })
    {}

    void SetCallback(Callback callback) { _callback = std::move(callback); }

    void PushFrame(Timepoint tp, size_t count) {
        for(size_t i = 1; i <= count; ++i) {
            PushPacket(tp, i == count);
        }
    }

    void PushPacket(Timepoint tp, bool marker = false) {
        auto packet = _allocator.Allocate(tp, marker);
        packet.SetSize(g_random.Int<size_t>(1, packet.GetCapacity()));
        _callback(std::move(packet));
    }

public:
    const Options _options;
    RtpAllocator _allocator;

    Callback _callback;
};

}
