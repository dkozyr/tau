#pragma once

#include "tau/ice/TransactionTracker.h"
#include "tau/ice/Candidate.h"
#include "tau/ice/Credentials.h"
#include "tau/ice/Constants.h"
#include "tau/stun/Header.h"
#include "tau/crypto/Hmac.h"
#include "tau/memory/Buffer.h"
#include <etl/unordered_map.h>
#include <etl/array.h>
#include <functional>

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
        etl::string_view log_ctx = {};
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

    void CreatePermission(const etl::ivector<IpAddress>& remote_ips);
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
    etl::array<uint8_t, stun::kTransactionIdSize> _transaction_id;
    std::optional<Endpoint> _relayed;

    struct Permission {
        bool done;
        Timepoint rto_tp;
    };
    etl::unordered_map<IpAddress, Permission, 4> _permissions;
    etl::unordered_map<Endpoint, etl::vector<Buffer, 16>, 4> _queue;

    etl::string<256> _realm;
    etl::string<256> _nonce;
    Timepoint _allocation_eol;
    std::optional<crypto::HmacHasher> _message_integrity_hasher;

    CandidateCallback _candidate_callback;
    Callback _send_callback;
    Callback _recv_callback;
};

}
