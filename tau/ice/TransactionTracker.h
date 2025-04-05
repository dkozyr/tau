#pragma once

#include "tau/memory/BufferView.h"
#include "tau/common/Clock.h"
#include <optional>
#include <unordered_map>

namespace tau::ice {

class TransactionTracker {
public:
    static constexpr size_t kTimeoutDefault = 500 * kMs;

    struct Result {
        Timepoint tp;
        size_t tag;
    };

public:
    TransactionTracker(Clock& clock, Timepoint timeout = kTimeoutDefault);

    void SetTransactionId(BufferView& stun_message_view, size_t tag);
    std::optional<Result> HasTransaction(uint32_t hash) const;
    void RemoveTransaction(uint32_t hash);
    Timepoint GetLastTimepoint(size_t tag) const;

    size_t GetCount() const;

private:
    void RemoveOldHashs();

private:
    Clock& _clock;
    const Timepoint _timeout;

    std::unordered_map<uint32_t, Result> _hash_storage;
    std::unordered_map<size_t, Timepoint> _tag_to_tp;
};

}
