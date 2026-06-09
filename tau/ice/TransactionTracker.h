#pragma once

#include "tau/memory/BufferView.h"
#include "tau/common/Clock.h"
#include <etl/unordered_map.h>
#include <optional>

namespace tau::ice {

class TransactionTracker {
public:
    static constexpr size_t kTimeoutDefault = 500 * kMs;
    static constexpr size_t kStorageCapacity = 16;

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

    etl::unordered_map<uint32_t, Result, kStorageCapacity> _hash_storage;
    etl::unordered_map<size_t, Timepoint, kStorageCapacity> _tag_to_tp;
};

}
