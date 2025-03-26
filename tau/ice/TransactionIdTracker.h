#pragma once

#include "tau/memory/BufferView.h"
#include "tau/asio/Common.h"
#include "tau/common/Clock.h"
#include <optional>
#include <unordered_map>

namespace tau::ice {

class TransactionIdTracker {
public:
    static constexpr size_t kTimeoutDefault = 500 * kMs;

    struct Result {
        asio_udp::endpoint remote;
        Timepoint tp;
    };

public:
    TransactionIdTracker(Clock& clock, Timepoint timeout = kTimeoutDefault);

    void SetTransactionId(BufferView& stun_message_view, asio_udp::endpoint remote);
    std::optional<Result> HasTransactionId(uint32_t hash) const;
    void RemoveTransactionId(uint32_t hash);
    Timepoint GetLastTimepoint(asio_udp::endpoint remote) const;

    size_t GetCount() const;

private:
    void RemoveOldHashs();

private:
    Clock& _clock;
    const Timepoint _timeout;

    std::unordered_map<uint32_t, Result> _hash_storage;
    std::unordered_map<asio_udp::endpoint, Timepoint> _endpoint_to_tp;
};

}
