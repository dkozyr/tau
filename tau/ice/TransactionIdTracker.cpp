#include "tau/ice/TransactionIdTracker.h"
#include "tau/stun/Header.h"
#include "tau/common/Container.h"

namespace tau::ice {

using Result = TransactionIdTracker::Result;

TransactionIdTracker::TransactionIdTracker(Clock& clock, Timepoint timeout)
    : _clock(clock)
    , _timeout(timeout)
{}

void TransactionIdTracker::SetTransactionId(BufferView& stun_message_view, asio_udp::endpoint remote) {
    RemoveOldHashs();

    auto transaction_id_ptr = stun_message_view.ptr + 2 * sizeof(uint32_t);
    while(true) {
        const auto id = stun::GenerateTransactionId(transaction_id_ptr);
        if(!Contains(_hash_storage, id)) {
            auto now = _clock.Now();
            _hash_storage[id] = Result{.remote = remote, .tp = now};
            _endpoint_to_tp[remote] = now;
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

Timepoint TransactionIdTracker::GetLastTimepoint(asio_udp::endpoint remote) const {
    auto it = _endpoint_to_tp.find(remote);
    if(it != _endpoint_to_tp.end()) {
        return it->second;
    }
    return _clock.Now() - 60 * kSec;
}

size_t TransactionIdTracker::GetCount() const {
    return _hash_storage.size();
}

void TransactionIdTracker::RemoveOldHashs() {
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
