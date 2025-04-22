
#pragma once

#include "tau/ice/CheckList.h"
#include "tau/ice/StunClient.h"
#include "tau/ice/TurnClient.h"

namespace tau::ice {

class Agent {
public:
    using StateCallback = std::function<void(State state)>;
    using CandidateCallback = CheckList::CandidateCallback;
    using SendCallback = CheckList::SendCallback;
    using NominatingStrategy = CheckList::NominatingStrategy;

    struct Dependencies {
        Clock& clock;
        Allocator& udp_allocator;
    };

    struct Options {
        Role role;
        Credentials credentials;
        std::vector<Endpoint> interfaces; // UDP only, only 1 endpoint (port) per IP, ordering is used as user preferences
        std::vector<Endpoint> stun_servers;
        std::unordered_map<Endpoint, PeerCredentials> turn_servers;
        NominatingStrategy nominating_strategy = NominatingStrategy::kBestValid;
        std::string log_ctx = {};
    };

public:
    Agent(Dependencies&& deps, Options&& options);
    ~Agent();

    void SetStateCallback(StateCallback callback);
    void SetSendCallback(SendCallback callback);
    void SetCandidateCallback(CandidateCallback callback);

    void Start();
    void Process();

    void RecvRemoteCandidate(std::string candidate);
    void Recv(size_t socket_idx, Endpoint remote, Buffer&& message);

    const CandidatePair& GetBestCandidatePair() const;

private:
    void InitStunClients(const std::vector<Endpoint>& stun_servers);
    void InitTurnClients(const std::unordered_map<Endpoint, PeerCredentials>& turn_servers);

private:
    Dependencies _deps;
    std::vector<Endpoint> _interfaces;
    const std::string _log_ctx;

    CheckList _check_list;
    std::vector<StunClient> _stun_clients;
    std::vector<TurnClient> _turn_clients;

    State _state = State::kWaiting;

    StateCallback _state_callback;
    SendCallback _send_callback;
};

}
