#pragma once

#include <tau/rtp/RtpAllocator.h>
#include <tau/video/av1/Obu.h>
#include <functional>
#include <vector>

namespace tau::rtp {

class Av1Packetizer {
public:
    using Callback = std::function<void(Buffer&&)>;

public:
    explicit Av1Packetizer(RtpAllocator& allocator);

    void SetCallback(Callback callback) { _callback = std::move(callback); }
    void StartSequence() { _new_sequence = true; }

    bool Process(const Buffer& access_unit);
    bool Process(const Buffer& unit, bool last);

private:
    bool Process(const std::vector<av1::Obu>& units, Timepoint tp, bool last, bool aggregate);
    size_t WritePayload(uint8_t* payload, const std::vector<av1::Obu>& units, size_t& unit_index, size_t& unit_offset);

    static std::vector<av1::Obu> ReadUnits(BufferViewConst view);

private:
    RtpAllocator& _allocator;
    const size_t _max_payload;

    bool _new_sequence = false;

    Callback _callback;
};

}
