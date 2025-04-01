#pragma once

#include "tau/ice/TransactionTracker.h"
#include "tau/ice/Candidate.h"
#include "tau/memory/Buffer.h"
#include "tau/asio/Common.h"
#include <functional>

namespace tau::ice {

class StunClient {
public:
    struct Dependencies {
        Clock& clock;
        Allocator& udp_allocator;
    };

    using CandidateCallback = std::function<void(Endpoint reflexive)>;
    using SendCallback = std::function<void(Endpoint remote, Buffer&& message)>;

public:
    StunClient(Dependencies&& deps, Endpoint server);

    void SetCandidateCallback(CandidateCallback callback) { _candidate_callback = std::move(callback); };
    void SetSendCallback(SendCallback callback) { _send_callback = std::move(callback); }

    void Process();
    void Recv(Buffer&& message);

    bool IsServerEndpoint(Endpoint remote) const;

private:
    void SendStunRequest();
    void OnStunResponse(const BufferViewConst& view);
    Timepoint WaitPeriodToNextRequest() const;

private:
    Dependencies _deps;
    const Endpoint _server;
    TransactionTracker _transaction_tracker;
    uint32_t _transaction_hash = 0;
    std::optional<Endpoint> _reflexive;

    CandidateCallback _candidate_callback;
    SendCallback _send_callback;
};

}
