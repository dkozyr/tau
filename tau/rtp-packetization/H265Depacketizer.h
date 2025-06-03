#pragma once

#include <tau/rtp/Frame.h>
#include <functional>
#include <optional>

namespace tau::rtp {

class H265Depacketizer {
public:
    static constexpr auto kNaluMaxSizeDefault = 0x1'0000;

    using Callback = std::function<void(Buffer&&)>;

public:
    explicit H265Depacketizer(Allocator& allocator);

    void SetCallback(Callback callback) { _callback = std::move(callback); }

    bool Process(Frame&& frame);

private:
    bool Process(BufferViewConst rtp_payload_view, Timepoint tp, bool last);
    bool ProcessSingle(BufferViewConst rtp_payload_view, Timepoint tp, bool last);
    bool ProcessFu(BufferViewConst rtp_payload_view, Timepoint tp, bool last);
    bool ProcessAp(BufferViewConst rtp_payload_view, Timepoint tp, bool last);

    bool ValidateFu(BufferViewConst payload_view) const;

    static size_t GetNaluMaxSize(const Frame& frame);

private:
    Allocator& _allocator;
    Callback _callback;

    size_t _nalu_max_size = kNaluMaxSizeDefault;
    std::optional<Buffer> _fu_nal_unit;
};

}
