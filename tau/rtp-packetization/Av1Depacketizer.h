#pragma once

#include <tau/rtp/Frame.h>
#include <functional>
#include <optional>

namespace tau::rtp {

class Av1Depacketizer {
public:
    static constexpr size_t kObuMaxSizeDefault = 1 * 1024 * 1024;

    using Callback = std::function<void(Buffer&&)>;

public:
    explicit Av1Depacketizer(Allocator& allocator, size_t max_unit_size = kObuMaxSizeDefault);

    void SetCallback(Callback callback) { _callback = std::move(callback); }
    bool Process(Frame&& frame);

private:
    bool Process(const Buffer& packet, bool last);
    bool ProcessPayload(BufferViewConst payload, Timepoint tp, bool last, bool marker);
    bool ProcessElement(BufferViewConst element, Timepoint tp, bool continuation, bool incomplete, std::optional<Buffer>& pending);
    bool Append(BufferViewConst element, Timepoint tp);
    bool Complete(BufferViewConst element, Timepoint tp, std::optional<Buffer>& pending);
    bool Fail();

private:
    Allocator& _allocator;
    const size_t _max_unit_size;

    struct PacketMetadata {
        uint32_t ssrc;
        uint32_t ts;
        uint16_t sn;

        bool operator==(const PacketMetadata& other) const {
            return (ssrc == other.ssrc) && (ts == other.ts) && (sn == other.sn);
        }
    };
    std::optional<PacketMetadata> _expected_packet;
    std::optional<Buffer> _fragment;

    Callback _callback;
};

}
