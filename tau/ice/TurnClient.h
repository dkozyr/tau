#pragma once

#include "tau/ice/TransactionTracker.h"
#include "tau/ice/Candidate.h"
#include "tau/ice/Credentials.h"
#include "tau/ice/Constants.h"
#include "tau/memory/Buffer.h"
#include "tau/asio/Common.h"
#include <functional>
#include <unordered_map>

namespace tau::ice {

class TurnClient {
public:
    static constexpr size_t kRefreshSecDefault = 600;

    struct Dependencies {
        Clock& clock;
        Allocator& udp_allocator;
    };

    struct Options {
        Endpoint server;
        PeerCredentials credentials;
        Timepoint start_delay = kRtoDefault / 2;
        std::string log_ctx = {};
    };

    using CandidateCallback = std::function<void(Endpoint reflexive)>;
    using Callback = std::function<void(Endpoint remote, Buffer&& message)>;

public:
    TurnClient(Dependencies&& deps, Options&& options);

    TurnClient(const TurnClient&) = delete;
    TurnClient(TurnClient&&) = default;

    void SetCandidateCallback(CandidateCallback callback) { _candidate_callback = std::move(callback); };
    void SetSendCallback(Callback callback) { _send_callback = std::move(callback); }
    void SetRecvCallback(Callback callback) { _recv_callback = std::move(callback); }

    void Process();
    void Recv(Buffer&& message);
    void Send(Buffer&& message, Endpoint remote);

    void CreatePermission(const std::vector<IpAddress>& remote_ips);
    bool HasPermission(IpAddress remote);
    void Stop();
    bool IsActive() const;

    bool IsServerEndpoint(Endpoint remote) const;

private:
    void ProcessPermissionsRto();

    void SendAllocationRequest(bool authenticated);
    void SendRefreshRequest(size_t refresh_sec = kRefreshSecDefault);
    void SendCreatePermissionRequest(IpAddress remote);
    void SendDataIndication(Buffer&& packet, Endpoint remote);

    void OnStunResponse(const BufferViewConst& view);
    void OnCreatePermissionResponse(uint32_t hash);
    void OnDataIndication(Buffer&& message);

    void UpdateMessageIntegrityPassword();

private:
    Dependencies _deps;
    const Options _options;
    Timepoint _next_request_tp;
    bool _stopped = false;
    TransactionTracker _transaction_tracker;
    uint32_t _transaction_hash = 0;
    std::vector<uint8_t> _transaction_id;
    std::optional<Endpoint> _relayed;

    struct Permission {
        bool done;
        Timepoint rto_tp;
    };
    std::unordered_map<IpAddress, Permission> _permissions;
    std::unordered_map<Endpoint, std::vector<Buffer>> _queue;

    std::string _realm;
    std::string _nonce;
    Timepoint _allocation_eol;
    std::string _message_integrity_password;

    CandidateCallback _candidate_callback;
    Callback _send_callback;
    Callback _recv_callback;
};

}
