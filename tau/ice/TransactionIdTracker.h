#pragma once

#include "tau/memory/BufferView.h"
#include "tau/common/Clock.h"
#include <optional>
#include <unordered_map>

namespace tau::ice {

class TransactionIdTracker {
public:
    static constexpr size_t kStunServerKeepAlivePeriod = 5 * kSec; //TODO: check it
    static constexpr size_t kConnectivityCheckPeriod = 50 * kMs;
    static constexpr size_t kConnectivityCheckTimeout = 10 * kConnectivityCheckPeriod;

    enum Type {
        kStunServer,
        kConnectivityCheck
    };

    struct Result {
        Type type;
        Timepoint tp;
    };

public:
    TransactionIdTracker(Clock& clock);

    bool IsTimeout(Type type) const;
    void SetTransactionId(BufferView& stun_message_view, Type type);
    std::optional<Result> HasTransactionId(uint32_t hash) const;
    void RemoveTransactionId(uint32_t hash);

    size_t GetCount() const;

private:
    void RemoveOldHashs();

private:
    Clock& _clock;

    std::unordered_map<uint32_t, Result> _hash_storage;
    std::unordered_map<Type, Timepoint> _type_to_tp;
};

}
