
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
    using MdnsEndpointCallback = CheckList::MdnsEndpointCallback;
    using NominatingStrategy = CheckList::NominatingStrategy;

    struct Dependencies {
        Clock& clock;
        Allocator& udp_allocator;
    };

    struct Options {
        Role role;
        Credentials credentials;
        //TODO: fix capacity
        etl::vector<Endpoint, 3> interfaces; // UDP only, only 1 endpoint (port) per IP, ordering is used as user preferences
        etl::vector<Endpoint, 2> stun_servers;
        etl::unordered_map<Endpoint, PeerCredentials, 3> turn_servers;
        NominatingStrategy nominating_strategy = NominatingStrategy::kBestValid;
        etl::string_view log_ctx = {};
    };

public:
    Agent(Dependencies&& deps, Options&& options);
    ~Agent();

    void SetStateCallback(StateCallback callback);
    void SetSendCallback(SendCallback callback);
    void SetMdnsEndpointCallback(MdnsEndpointCallback callback);
    void SetCandidateCallback(CandidateCallback callback);

    void Start();
    void Process();

    void RecvRemoteCandidate(CandidateStr candidate);
    void Recv(size_t socket_idx, Endpoint remote, Buffer&& message);

    const CandidatePair& GetBestCandidatePair() const;

private:
    void InitStunClients(const etl::ivector<Endpoint>& stun_servers);
    void InitTurnClients(const etl::iunordered_map<Endpoint, PeerCredentials>& turn_servers);

private:
    Dependencies _deps;
    etl::vector<Endpoint, 3> _interfaces;
    const etl::string_view _log_ctx;

    CheckList _check_list;
    etl::vector<StunClient, 2 * 3> _stun_clients;
    etl::vector<TurnClient, 3 * 3> _turn_clients;

    State _state = State::kWaiting;

    StateCallback _state_callback;
    SendCallback _send_callback;
};

}
