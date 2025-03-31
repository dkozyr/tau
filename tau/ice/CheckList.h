#pragma once

#include "tau/ice/TransactionTracker.h"
#include "tau/ice/CandidatePair.h"
#include "tau/ice/Credentials.h"
#include "tau/ice/State.h"
#include "tau/memory/Buffer.h"
#include <functional>
#include <unordered_map>

namespace tau::ice {

class CheckList {
public:
    enum NominatingStrategy {
        kBestValid,
        kFirstValid,
    };

    struct Dependencies {
        Clock& clock;
        Allocator& udp_allocator;
    };

    struct Options {
        Role role;
        Credentials credentials;
        //TODO: rename to local_endpoints?
        std::vector<Endpoint> sockets; // UDP only, only 1 endpoint (port) per IP, ordering is used as user preferences
        Endpoint stun_server;
        // std::vector<Endpoint> turn_servers; // TODO: impl
        NominatingStrategy nominating_strategy = NominatingStrategy::kBestValid;
        std::string log_ctx = {};
    };

    using StateCallback = std::function<void(State state)>;
    using CandidateCallback = std::function<void(std::string candidate)>;
    using SendCallback = std::function<void(size_t socket_idx, Endpoint remote, Buffer&& message)>;

public:
    CheckList(Dependencies&& deps, Options&& options);
    ~CheckList();

    void SetStateCallback(StateCallback callback) { _state_callback = std::move(callback); }
    void SetSendCallback(SendCallback callback) { _send_callback = std::move(callback); }
    void SetCandidateCallback(CandidateCallback callback) { _candidate_callback = std::move(callback); }

    void Start();
    void Process();

    void RecvRemoteCandidate(std::string candidate);
    void Recv(size_t socket_idx, Endpoint remote, Buffer&& message);

    State GetState() const;

private:
    void ProcessLocalCandidate(CandidateType type, size_t socket_idx, Endpoint endpoint);

    void Nominating();
    void ProcessConnectivityChecks(size_t socket_idx);
    void SendStunRequest(size_t socket_idx, std::optional<size_t> pair_id, Endpoint remote, bool authenticated = true, bool nominating = false);
    void OnStunResponse(const BufferViewConst& view, size_t socket_idx, Endpoint remote);
    void OnStunRequest(Buffer&& message, const BufferViewConst& view, size_t socket_idx, Endpoint remote);

    void PrunePairs();
    void AddPair(const Candidate& local, const Candidate& remote);
    bool SetPairState(size_t id, CandidatePair::State state);
    CandidatePair& GetPairById(size_t id); 

    static std::optional<size_t> FindCandidateByEndpoint(const Candidates& candidates, Endpoint endpoint);

private:
    Dependencies _deps;
    const Role _role; // NOTE: role switching isn't supported
    const Credentials _credentials;
    const NominatingStrategy _nominating_strategy;
    const std::string _log_ctx;

    std::vector<Endpoint> _sockets; //TODO: local_endpoints?
    Endpoint _stun_server;
    // std::vector<Endpoint> _turn_servers; // TODO: impl

    std::unordered_map<size_t, TransactionTracker> _transcation_trackers;

    Candidates _local_candidates;
    Candidates _remote_candidates;

    CandidatePairs _pairs;
    size_t _next_pair_id = 1;
    Timepoint _last_ta_tp;

    StateCallback _state_callback;
    CandidateCallback _candidate_callback;
    SendCallback _send_callback;
};

}
