#include "tau/ice/TransactionIdTracker.h"
#include "tau/stun/Header.h"
#include "tau/common/Container.h"

namespace tau::ice {

using Result = TransactionIdTracker::Result;

TransactionIdTracker::TransactionIdTracker(Clock& clock)
    : _clock(clock)
    , _type_to_tp{
        {Type::kStunServer,        _clock.Now() - 60 * kSec},
        {Type::kConnectivityCheck, _clock.Now() - 60 * kSec}
    }
{}

bool TransactionIdTracker::IsTimeout(Type type) const {
    auto now = _clock.Now();
    auto last_tp = _type_to_tp.at(type);
    switch(type) {
        case Type::kStunServer:        return (now >= last_tp + kStunServerKeepAlivePeriod);
        case Type::kConnectivityCheck: return (now >= last_tp + kConnectivityCheckPeriod);
    }
    return false;
}

void TransactionIdTracker::SetTransactionId(BufferView& stun_message_view, Type type) {
    RemoveOldHashs();

    auto transaction_id_ptr = stun_message_view.ptr + 2 * sizeof(uint32_t);
    while(true) {
        const auto id = stun::GenerateTransactionId(transaction_id_ptr);
        if(!Contains(_hash_storage, id)) {
            auto now = _clock.Now();
            _hash_storage[id] = Result{.type = type, .tp = now};
            _type_to_tp[type] = now;
            break;
        }
    }
}

std::optional<Result> TransactionIdTracker::HasTransactionId(uint32_t hash) const {
    auto it = _hash_storage.find(hash);
    if(it != _hash_storage.end()) {
        return it->second;
    }
    return std::nullopt;
}

void TransactionIdTracker::RemoveTransactionId(uint32_t hash) {
    _hash_storage.erase(hash);
    RemoveOldHashs();
}

size_t TransactionIdTracker::GetCount() const {
    return _hash_storage.size();
}

void TransactionIdTracker::RemoveOldHashs() {
    auto now = _clock.Now();
    for(auto it = _hash_storage.begin(); it != _hash_storage.end(); ) {
        const auto& result = it->second;
        if(now >= result.tp + kConnectivityCheckTimeout) {
            it = _hash_storage.erase(it);
        } else {
            it++;
        }
    }
}

}
