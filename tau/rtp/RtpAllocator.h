#pragma once

#include "tau/memory/PoolAllocator.h"
#include "tau/memory/Buffer.h"
#include "tau/rtp/Writer.h"
#include "tau/rtp/TsConverter.h"
#include "tau/rtp/Constants.h"

namespace rtp {

class RtpAllocator {
public:
    struct Options {
        Writer::Options header;
        Timepoint base_tp;
        size_t clock_rate;
        size_t size = 1500; //TODO: name constant
    };

public:
    explicit RtpAllocator(Options&& options)
        : _options(std::move(options))
        , _ts_producer(TsConverter::Options{
            .rate = _options.clock_rate,
            .ts_base = _options.header.ts,
            .tp_base = _options.base_tp
        })
        , _pool(_options.size)
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

    size_t MaxRtpPayload() const {
        //TODO: do we need to subtract TURN header size?
        constexpr size_t kSrtpMaxAuthSize = 16;
        return _options.size - kFixedHeaderSize
               - HeaderExtensionSize(_options.header.extension_length_in_words)
               - kSrtpMaxAuthSize;
    }

private:
    Options _options;
    TsConverter _ts_producer;
    PoolAllocator _pool;
};

}
