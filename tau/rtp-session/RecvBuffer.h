#pragma once

#include <tau/memory/Buffer.h>
#include <etl/vector.h>
#include <etl/set.h>
#include <functional>
#include <optional>

namespace tau::rtp::session {

class RecvBuffer {
public:
    static constexpr size_t kDefaultSize = 256;
    static constexpr size_t kBufferCapacity = 256;

    struct Stats {
        uint64_t packets = 0;
        uint64_t discarded = 0;
        uint64_t lost = 0;
        uint64_t bytes = 0;
    };

    enum PacketType {
        kOk,
        kDiscarded,
        kReset
    };

    using Sns = etl::set<uint16_t, 32>;

    using Callback = std::function<void(Buffer&&)>;

public:
    explicit RecvBuffer(size_t size = kDefaultSize);

    void SetCallback(Callback callback) { _callback = std::move(callback); }

    PacketType Push(Buffer&& packet, uint16_t sn);
    void Flush();

    const Sns& GetSnsToRecover() const { return _sns_to_recover; }
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
    const size_t _size;

    bool _first_packet = true;
    uint16_t _sn_next = 0;
    std::optional<uint16_t> _sn_end;

    size_t _index = 0;
    etl::vector<std::optional<Buffer>, kBufferCapacity> _packets;
    Sns _sns_to_recover;

    Callback _callback;
    Stats _stats;
};

}
