#pragma once

#include "tau/memory/PoolAllocator.h"
#include "tau/memory/Buffer.h"
#include "tau/rtp/Writer.h"
#include "tau/rtp/Constants.h"
#include "tau/common/Log.h"

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
    RtpAllocator(Options&& options)
        : _options(std::move(options))
        , _base_rtp_tp(_options.header.ts)
        , _pool(_options.size)
    {}

    Buffer Allocate(Timepoint tp, bool marker = false) {
        auto packet = Buffer::Create(_pool);
        _options.header.ts = CalcTs(tp);
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
    uint32_t CalcTs(Timepoint tp) const {
        return _base_rtp_tp + static_cast<uint32_t>((tp - _options.base_tp) * _options.clock_rate / kSec);
    }

private:
    Options _options;
    const uint32_t _base_rtp_tp;
    PoolAllocator _pool;
};

}
