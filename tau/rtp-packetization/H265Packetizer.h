#pragma once

#include <tau/rtp/RtpAllocator.h>
#include <functional>

namespace tau::rtp {

class H265Packetizer {
public:
    using FuHeader = uint8_t;
    using Callback = std::function<void(Buffer&&)>;

public:
    explicit H265Packetizer(RtpAllocator& allocator);

    void SetCallback(Callback callback) { _callback = std::move(callback); }

    bool Process(const Buffer& nal_unit, bool last);

private:
    void ProcessSingle(const BufferViewConst& view, Timepoint tp, bool last);
    void ProcessFu(const BufferViewConst& view, Timepoint tp, bool last);

private:
    RtpAllocator& _allocator;
    const size_t _max_payload;
    Callback _callback;
};

}
