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

    using CandidateCallback = std::function<void(CandidateType type, Endpoint reflexive)>;
    using SendCallback = std::function<void(Endpoint remote, Buffer&& message)>;

public:
    TurnClient(Dependencies&& deps, Options&& options);

    void SetCandidateCallback(CandidateCallback callback) { _candidate_callback = std::move(callback); };
    void SetSendCallback(SendCallback callback) { _send_callback = std::move(callback); }

    void Process();
    void Recv(Buffer&& message);
    void Send(Buffer&& message, Endpoint remote);

    void CreatePermission(asio_ip::address remote);
    bool HasPermission(asio_ip::address remote);
    void Stop();

    bool IsServerEndpoint(Endpoint remote) const;

private:
    void ProcessPermissionsRto();

    void SendAllocationRequest(bool authenticated);
    void SendRefreshRequest(size_t refresh_sec = kRefreshSecDefault);
    void SendCreatePermissionRequest(asio_ip::address remote);

    void OnStunResponse(const BufferViewConst& view);
    void OnCreatePermissionResponse(uint32_t hash);

    void UpdateMessageIntegrityPassword();

private:
    Dependencies _deps;
    const Options _options;
    Timepoint _next_request_tp;
    TransactionTracker _transaction_tracker;
    uint32_t _transaction_hash = 0;
    std::vector<uint8_t> _transaction_id;
    std::optional<Endpoint> _peer;
    std::optional<Endpoint> _relayed;

    struct Permission {
        bool done;
        Timepoint rto_tp;
    };
    //TODO: rename asio_ip::address?
    std::unordered_map<asio_ip::address, Permission> _permissions;

    std::string _realm;
    std::string _nonce;
    Timepoint _allocation_eol;
    std::string _message_integrity_password;

    CandidateCallback _candidate_callback;
    SendCallback _send_callback;
};

}
