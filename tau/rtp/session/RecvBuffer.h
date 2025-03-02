#pragma once

#include "tau/memory/Buffer.h"
#include <functional>
#include <vector>
#include <optional>

namespace rtp::session {

class RecvBuffer {
public:
    static constexpr size_t kDefaultSize = 256;

    struct Stats {
        size_t packets = 0;
        size_t discarded = 0;
        size_t lost = 0;
        size_t bytes = 0;
    };

    enum PacketType {
        kOk,
        kDiscarded,
        kReset
    };

    using Callback = std::function<void(Buffer&&)>;

public:
    explicit RecvBuffer(size_t size = kDefaultSize);

    void SetCallback(Callback callback) { _callback = std::move(callback); }

    PacketType Push(Buffer&& packet, uint16_t sn);
    void Flush();

    const Stats& GetStats() const { return _stats; }

private:
    PacketType InsertPacket(Buffer&& packet, uint16_t sn);
    PacketType OnInRangePacket(Buffer&& packet, uint16_t sn);
    PacketType OnOrderedPacket(Buffer&& packet, uint16_t sn);
    void DoReset(Buffer&& packet, uint16_t sn);

    void SendAndProcessNext();
    void IncreaseSnState();
    size_t GetIndexBySn(uint16_t sn) const;

private:
    const uint16_t _size;

    bool _first_packet = true;
    uint16_t _sn_next = 0;
    uint16_t _sn_end = 0;

    uint16_t _index = 0;
    std::vector<std::optional<Buffer>> _packets;

    Callback _callback;
    Stats _stats;
};

}
