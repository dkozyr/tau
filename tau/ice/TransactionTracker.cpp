#include "tau/ice/TransactionTracker.h"
#include "tau/stun/Header.h"
#include "tau/common/Container.h"

namespace tau::ice {

using Result = TransactionTracker::Result;

TransactionTracker::TransactionTracker(Clock& clock, Timepoint timeout)
    : _clock(clock)
    , _timeout(timeout)
{}

void TransactionTracker::SetTransactionId(BufferView& stun_message_view, std::optional<size_t> pair_id) {
    RemoveOldHashs();

    auto transaction_id_ptr = stun_message_view.ptr + 2 * sizeof(uint32_t);
    while(true) {
        const auto id = stun::GenerateTransactionId(transaction_id_ptr);
        if(!Contains(_hash_storage, id)) {
            auto now = _clock.Now();
            _hash_storage[id] = Result{ .tp = now, .pair_id = pair_id};
            if(pair_id) {
                _pair_id_to_tp[*pair_id] = now;
            }
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

Timepoint TransactionTracker::GetLastTimepoint(size_t pair_id) const {
    auto it = _pair_id_to_tp.find(pair_id);
    if(it != _pair_id_to_tp.end()) {
        return it->second;
    }
    return _clock.Now() - 60 * kSec;
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
