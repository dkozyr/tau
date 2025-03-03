#pragma once

#include "tau/memory/Buffer.h"
#include <functional>
#include <vector>
#include <optional>

namespace rtp::session {

class SendBuffer {
public:
    static constexpr size_t kDefaultSize = 256;

    struct Stats {
        size_t packets = 0;
        size_t rtx = 0;
        size_t bytes = 0;
    };

    using Callback = std::function<void(Buffer&&)>;

public:
    explicit SendBuffer(size_t size = kDefaultSize);

    void SetCallback(Callback callback) { _callback = std::move(callback); }

    void Push(Buffer&& packet, uint16_t sn);
    bool SendRtx(uint16_t sn);

    const Stats& GetStats() const { return _stats; }

private:
    size_t GetIndexBySn(uint16_t sn) const;

private:
    const size_t _size;

    uint16_t _sn_begin;
    size_t _index = 0;
    std::vector<Buffer> _packets;

    Callback _callback;
    Stats _stats;
};

}
