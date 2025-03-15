#pragma once

#include <tau/rtp/Frame.h>
#include <tau/video/h264/Nalu.h>
#include <functional>
#include <optional>

namespace tau::rtp {

class H264Depacketizer {
public:
    static constexpr auto kNaluMaxSizeDefault = 0x1'0000;

    using Callback = std::function<void(Buffer&&)>;

public:
    explicit H264Depacketizer(Allocator& allocator);

    void SetCallback(Callback callback) { _callback = std::move(callback); }

    bool Process(Frame&& frame);

private:
    bool Process(BufferViewConst rtp_payload_view);
    bool ProcessSingle(BufferViewConst rtp_payload_view);
    bool ProcessFuA(BufferViewConst rtp_payload_view);
    bool ProcessStapA(BufferViewConst rtp_payload_view);

    bool ValidateFuA(BufferViewConst payload_view) const;

    static size_t GetNaluMaxSize(const Frame& frame);

private:
    Allocator& _allocator;
    Callback _callback;

    size_t _nalu_max_size = kNaluMaxSizeDefault;
    std::optional<Buffer> _fua_nal_unit;
};

}
