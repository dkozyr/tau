
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

    static constexpr size_t kInterfaceMaxCount = 3;
    static constexpr size_t kStunMaxCount = 2;
    static constexpr size_t kTurnMaxCount = 3;

    struct Dependencies {
        Clock& clock;
        Allocator& udp_allocator;
    };

    struct Options {
        Role role;
        Credentials credentials;
        etl::vector<Endpoint, kInterfaceMaxCount> interfaces; // UDP only, only 1 endpoint (port) per IP, ordering is used as user preferences
        etl::vector<Endpoint, kStunMaxCount> stun_servers;
        etl::unordered_map<Endpoint, PeerCredentials, kTurnMaxCount> turn_servers;
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
    etl::vector<Endpoint, kInterfaceMaxCount> _interfaces;
    const etl::string_view _log_ctx;

    CheckList _check_list;
    etl::vector<StunClient, kStunMaxCount * kInterfaceMaxCount> _stun_clients;
    etl::vector<TurnClient, kTurnMaxCount * kInterfaceMaxCount> _turn_clients;

    State _state = State::kWaiting;

    StateCallback _state_callback;
    SendCallback _send_callback;
};

}
