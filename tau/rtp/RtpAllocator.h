#pragma once

#include "tau/memory/Allocator.h"
#include "tau/memory/Buffer.h"
#include "tau/rtp/Writer.h"
#include "tau/rtp/TsConverter.h"
#include "tau/rtp/Constants.h"

namespace tau::rtp {

class RtpAllocator {
public:
    struct Options {
        Writer::Options header;
        Timepoint base_tp;
        size_t clock_rate;
    };

public:
    explicit RtpAllocator(Allocator& allocator, Options&& options)
        : _pool(allocator)
        , _options(std::move(options))
        , _ts_producer(TsConverter::Options{
            .rate = _options.clock_rate,
            .ts_base = _options.header.ts,
            .tp_base = _options.base_tp
        })
    {}

    Buffer Allocate(Timepoint tp, bool marker = false) {
        auto packet = Buffer::Create(_pool, Buffer::Info{.tp = tp});
        _options.header.ts = _ts_producer.FromTp(tp);
        _options.header.marker = marker;
        auto result = Writer::Write(packet.GetViewWithCapacity(), _options.header);
        if(result.size == 0) {
            throw -1;
        }
        _options.header.sn++;
        packet.SetSize(result.size);
        return packet;
    }

    void Deallocate(Buffer&& buffer) {
        _pool.Deallocate(buffer.GetView().ptr);
    }

    size_t MaxRtpPayload() const { //TODO: TURN, SRTP, MTU options
        constexpr size_t kSrtpMaxAuthSize = 16;
        constexpr size_t kTurnMessageHeaderSize = 20 + 12; // Header + XOR-MAPPED-ADDRESS(IpV4)
        return _pool.GetChunkSize() - kFixedHeaderSize
               - HeaderExtensionSize(_options.header.extension_length_in_words)
               - kSrtpMaxAuthSize - kTurnMessageHeaderSize;
    }

private:
    Allocator& _pool;
    Options _options;
    TsConverter _ts_producer;
};

}
