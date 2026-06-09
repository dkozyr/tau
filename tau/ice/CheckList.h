#pragma once

#include "tau/ice/TransactionTracker.h"
#include "tau/ice/CandidatePair.h"
#include "tau/ice/Credentials.h"
#include "tau/ice/State.h"
#include "tau/crypto/Hmac.h"
#include "tau/memory/Buffer.h"
#include <etl/vector.h>
#include <etl/unordered_map.h>
#include <functional>

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
        //TODO: rename to local_endpoints? //TODO: fix capacity
        etl::vector<Endpoint, 3> sockets; // UDP only, only 1 endpoint (port) per IP, ordering is used as user preferences
        NominatingStrategy nominating_strategy = NominatingStrategy::kBestValid;
        etl::string_view log_ctx = {};
    };

    using CandidateCallback = std::function<void(CandidateStr candidate)>;
    using SendCallback = std::function<void(size_t socket_idx, Endpoint remote, Buffer&& message)>;
    using MdnsEndpointCallback = std::function<etl::string<36 + 1 + 5>(Endpoint local_endpoint)>; //TODO: name string type, uuid size = 36, "uuid.local"

public:
    CheckList(Dependencies&& deps, Options&& options);
    ~CheckList();

    void SetSendCallback(SendCallback callback) { _send_callback = std::move(callback); }
    void SetCandidateCallback(CandidateCallback callback) { _candidate_callback = std::move(callback); }
    void SetMdnsEndpointCallback(MdnsEndpointCallback callback) { _mdns_endpoint_callback = std::move(callback); }

    void Start();
    void Process();

    void AddLocalCandidate(CandidateType type, size_t socket_idx, Endpoint remote);
    void RecvRemoteCandidate(CandidateStr candidate);
    void Recv(size_t socket_idx, Endpoint remote, Buffer&& message);

    State GetState() const;
    const CandidatePair& GetBestCandidatePair() const;

private:
    void Nominating();
    void ProcessConnectivityChecks(size_t socket_idx);
    void SendStunRequest(size_t socket_idx, size_t pair_id, Endpoint remote, bool nominating = false);
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
    const etl::string_view _log_ctx;

    etl::vector<Endpoint, 3> _sockets; //TODO: local_endpoints? //TODO: fix capacity
    etl::unordered_map<size_t, TransactionTracker, 4> _transcation_trackers;

    Candidates _local_candidates;
    Candidates _remote_candidates;
    crypto::HmacHasher _hmac_hasher_local;
    crypto::HmacHasher _hmac_hasher_remote;

    CandidatePairs _pairs;
    size_t _next_pair_id = 1;
    Timepoint _last_ta_tp;

    CandidateCallback _candidate_callback;
    SendCallback _send_callback;
    MdnsEndpointCallback _mdns_endpoint_callback;
};

}
