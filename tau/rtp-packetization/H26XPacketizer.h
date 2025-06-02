#pragma once

#include <tau/rtp/RtpAllocator.h>
#include <functional>

namespace tau::rtp {

class H26XPacketizer {
public:
    struct Options {
        uint8_t nalu_header_size;
        uint8_t fragmented_nalu_type;
        std::function<bool(const BufferViewConst& view)> validate_header;
        std::function<uint8_t(const BufferViewConst& view)> get_nalu_type;
        std::function<void(uint8_t* ptr, uint8_t type)> set_nalu_type;
    };

    using Callback = std::function<void(Buffer&&)>;

public:
    explicit H26XPacketizer(RtpAllocator& allocator, Options&& options);

    void SetCallback(Callback callback) { _callback = std::move(callback); }

    bool Process(const Buffer& nal_unit, bool last);

private:
    void ProcessSingle(const BufferViewConst& view, Timepoint tp, bool last);
    void ProcessFragmented(const BufferViewConst& view, Timepoint tp, bool last);

    static uint8_t CreateFragmentedHeader(bool start, bool end, uint8_t type);

private:
    RtpAllocator& _allocator;
    const Options _options;
    const size_t _max_payload;
    Callback _callback;
};

}
