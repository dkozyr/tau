#include "tau/ice/TransactionTracker.h"
#include "tau/stun/Header.h"
#include "tau/common/Container.h"

namespace tau::ice {

using Result = TransactionTracker::Result;

TransactionTracker::TransactionTracker(Clock& clock, Timepoint timeout)
    : _clock(clock)
    , _timeout(timeout)
{}

void TransactionTracker::SetTransactionId(BufferView& stun_message_view, size_t tag) {
    RemoveOldHashs();

    auto transaction_id_ptr = stun_message_view.ptr + 2 * sizeof(uint32_t);
    while(true) {
        const auto hash = stun::GenerateTransactionId(transaction_id_ptr);
        if(!Contains(_hash_storage, hash)) {
            auto now = _clock.Now();
            _hash_storage[hash] = Result{.tp = now, .tag = tag};
            _tag_to_tp[tag] = now;
            break;
        }
    }
}

std::optional<Result> TransactionTracker::HasTransaction(uint32_t hash) const {
    auto it = _hash_storage.find(hash);
    if(it != _hash_storage.end()) {
        return it->second;
    }
    return std::nullopt;
}

void TransactionTracker::RemoveTransaction(uint32_t hash) {
    _hash_storage.erase(hash);
    RemoveOldHashs();
}

Timepoint TransactionTracker::GetLastTimepoint(size_t tag) const {
    auto it = _tag_to_tp.find(tag);
    if(it != _tag_to_tp.end()) {
        return it->second;
    }
    return _clock.Now() - 10 * kMin;
}

size_t TransactionTracker::GetCount() const {
    return _hash_storage.size();
}

void TransactionTracker::RemoveOldHashs() {
    auto now = _clock.Now();
    for(auto it = _hash_storage.begin(); it != _hash_storage.end(); ) {
        const auto& result = it->second;
        if(now >= result.tp + _timeout) {
            it = _hash_storage.erase(it);
        } else {
            it++;
        }
    }
}

}
