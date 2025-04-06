#include "tau/ice/Agent.h"

namespace tau::ice {

Agent::Agent(Dependencies&& deps, Options&& options)
    : _deps(std::move(deps))
    , _interfaces(options.interfaces)
    , _log_ctx(options.log_ctx)
    , _check_list(
        CheckList::Dependencies{
            .clock = _deps.clock,
            .udp_allocator = _deps.udp_allocator
        },
        CheckList::Options{
            .role = options.role,
            .credentials = options.credentials,
            .sockets = std::move(options.interfaces),
            .nominating_strategy = options.nominating_strategy,
            .log_ctx = options.log_ctx
        }
    )
{
    _check_list.SetSendCallback([this](size_t socket_idx, Endpoint remote, Buffer&& message) {
        _send_callback(socket_idx, remote, std::move(message));
    });
    InitStunClients(options.stun_servers);
    InitTurnClients(options.turn_servers);
}

Agent::~Agent() {
    //TODO: impl
}

void Agent::SetStateCallback(StateCallback callback) {
    _state_callback = std::move(callback);
}

void Agent::SetSendCallback(SendCallback callback) {
    _send_callback = std::move(callback);
}

void Agent::SetCandidateCallback(CandidateCallback callback) {
    _check_list.SetCandidateCallback(std::move(callback));
}

void Agent::Start() {
    _check_list.Start();
}

void Agent::Process() {
    for(auto& stun_client : _stun_clients) { stun_client.Process(); }
    for(auto& turn_client : _turn_clients) { turn_client.Process(); }
    _check_list.Process();
}

void Agent::RecvRemoteCandidate(std::string candidate) {
    _check_list.RecvRemoteCandidate(std::move(candidate));
}

void Agent::Recv(size_t socket_idx, Endpoint remote, Buffer&& message) {
    for(size_t i = socket_idx; i < _turn_clients.size(); i += _interfaces.size()) {
        auto& turn_client = _turn_clients[i];
        if(turn_client.IsServerEndpoint(remote)) {
            turn_client.Recv(std::move(message));
            return;
        }
    }
    for(size_t i = socket_idx; i < _stun_clients.size(); i += _interfaces.size()) {
        auto& stun_client = _stun_clients[i];
        if(stun_client.IsServerEndpoint(remote)) {
            stun_client.Recv(std::move(message));
            return;
        }
    }
    _check_list.Recv(socket_idx, remote, std::move(message));
}

void Agent::InitStunClients(const std::vector<Endpoint>& stun_servers) {
    for(auto& endpoint : stun_servers) {
        for(size_t i = 0; i < _interfaces.size(); ++i) {
            _stun_clients.emplace_back(
                StunClient::Dependencies{
                    .clock = _deps.clock,
                    .udp_allocator = _deps.udp_allocator
                },
                endpoint
            );
            auto& stun_client = _stun_clients.back();
            stun_client.SetCandidateCallback([this, socket_idx = i](Endpoint reflexive) {
                _check_list.AddLocalCandidate(CandidateType::kServRefl, socket_idx, reflexive);
            });
            stun_client.SetSendCallback([this, socket_idx = i](Endpoint remote, Buffer&& message) {
                _send_callback(socket_idx, remote, std::move(message));
            });
        }
    }
}

void Agent::InitTurnClients(const std::unordered_map<Endpoint, PeerCredentials>& turn_servers) {
    for(auto& [endpoint, credentials] : turn_servers) {
        for(size_t i = 0; i < _interfaces.size(); ++i) {
            _turn_clients.emplace_back(
                TurnClient::Dependencies{
                    .clock = _deps.clock,
                    .udp_allocator = _deps.udp_allocator
                },
                TurnClient::Options{
                    .server = endpoint,
                    .credentials = credentials,
                    .log_ctx = _log_ctx
                }
            );
            auto& turn_client = _turn_clients.back();
            turn_client.SetCandidateCallback([this, socket_idx = i](Endpoint relayed) {
                _check_list.AddLocalCandidate(CandidateType::kRelayed, socket_idx, relayed);
            });
            turn_client.SetSendCallback([this, socket_idx = i](Endpoint remote, Buffer&& message) {
                _send_callback(socket_idx, remote, std::move(message));
            });
        }
    }
}

}
