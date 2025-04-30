#include "tau/ice/CheckList.h"
#include "tau/ice/Constants.h"
#include "tau/stun/Reader.h"
#include "tau/stun/Writer.h"
#include "tau/stun/attribute/XorMappedAddress.h"
#include "tau/stun/attribute/DataUint32.h"
#include "tau/stun/attribute/UseCandidate.h"
#include "tau/stun/attribute/IceRole.h"
#include "tau/stun/attribute/ByteString.h"
#include "tau/stun/attribute/MessageIntegrity.h"
#include "tau/stun/attribute/Fingerprint.h"
#include "tau/sdp/line/attribute/Candidate.h"
#include "tau/common/Container.h"
#include "tau/common/String.h"
#include "tau/common/Log.h"
#include <sstream>
#include <cctype>

namespace tau::ice {

using namespace stun;
using namespace stun::attribute;

CheckList::CheckList(Dependencies&& deps, Options&& options)
    : _deps(std::move(deps))
    , _role(options.role)
    , _credentials(options.credentials)
    , _nominating_strategy(options.nominating_strategy)
    , _log_ctx(std::move(options.log_ctx))
    , _sockets(std::move(options.sockets))
    , _last_ta_tp(_deps.clock.Now() - kTaDefault) {
    for(size_t i = 0; i < _sockets.size(); ++i) {
        _transcation_trackers.insert({i, TransactionTracker(_deps.clock)});
    }
}

CheckList::~CheckList() {
    TAU_LOG_DEBUG(_log_ctx << "Candidate pairs:");
    for(auto& pair : _pairs) {
        TAU_LOG_DEBUG(_log_ctx << "id: " << pair.id << ", local: " << (size_t)pair.local.type << ", remote: " << (size_t)pair.remote.type
            << ", priority: " << pair.priority << ", socket: " << pair.local.socket_idx.value()
            << ", remote: " << pair.remote.endpoint << ", state: " << pair.state << ", attempts: " << pair.attempts_count);
    }
}

void CheckList::Start() {
    for(size_t i = 0; i < _sockets.size(); ++i) {
        AddLocalCandidate(CandidateType::kHost, i, _sockets[i]);
    }

    for(auto& remote : _remote_candidates) {
        for(auto& local : _local_candidates) {
            AddPair(local, remote);
        }
    }
    PrunePairs();
}

void CheckList::Process() {
    const auto now = _deps.clock.Now();
    if(now < _last_ta_tp + kTaDefault) {
        return;
    }
    _last_ta_tp = now;

    for(auto& [socket_idx, _] : _transcation_trackers) {
        ProcessConnectivityChecks(socket_idx);
    }
    Nominating();
}

void CheckList::AddLocalCandidate(CandidateType type, size_t socket_idx, Endpoint endpoint) {
    _local_candidates.push_back(Candidate{
        .type = type,
        .priority = Priority(type, socket_idx),
        .endpoint = endpoint,
        .socket_idx = socket_idx
    });
    if(type == CandidateType::kRelayed) {
        _transcation_trackers.insert({socket_idx, TransactionTracker(_deps.clock)});
        for(auto& remote : _remote_candidates) {
            if(remote.type != CandidateType::kPeerRefl) {
                AddPair(_local_candidates.back(), remote);
            }
        }
    }
    if((type == CandidateType::kHost) && _mdns_endpoint_callback) {
        auto mdns_name = _mdns_endpoint_callback(endpoint);
        _candidate_callback(ToCandidateAttributeString(type, socket_idx, endpoint, mdns_name));
    } else if(type != CandidateType::kPeerRefl) {
        _candidate_callback(ToCandidateAttributeString(type, socket_idx, endpoint));
    }
}

void CheckList::RecvRemoteCandidate(std::string candidate) {
    ToLowerCase(candidate);
    TAU_LOG_INFO(_log_ctx << "Candidate: " << candidate);
    if(!sdp::attribute::CandidateReader::Validate(candidate)) {
        TAU_LOG_WARNING(_log_ctx << "Invalid remote candidate: " << candidate);
        return;
    }
    if(sdp::attribute::CandidateReader::GetTransport(candidate) != "udp") {
        TAU_LOG_WARNING(_log_ctx << "Unsupported transport: " << sdp::attribute::CandidateReader::GetTransport(candidate));
        return;
    }
    if(sdp::attribute::CandidateReader::GetComponentId(candidate) != 1) {
        TAU_LOG_WARNING(_log_ctx << "Wrong component transport: " << sdp::attribute::CandidateReader::GetComponentId(candidate));
        return;
    }
    auto address = sdp::attribute::CandidateReader::GetAddress(candidate);
    auto port = sdp::attribute::CandidateReader::GetPort(candidate);
    Endpoint endpoint{asio_ip::make_address(address), port};
    if(!endpoint.address().is_v4()) {
        return;
    }
    _remote_candidates.emplace_back(Candidate{
        .type = CandidateTypeFromString(sdp::attribute::CandidateReader::GetType(candidate)),
        .priority = sdp::attribute::CandidateReader::GetPriority(candidate),
        .endpoint = endpoint
    });
    for(auto& local : _local_candidates) {
        if((local.type == CandidateType::kHost) || (local.type == CandidateType::kRelayed)) {
            AddPair(local, _remote_candidates.back());
        }
    }
}

void CheckList::Recv(size_t socket_idx, Endpoint remote, Buffer&& message) {
    auto view = ToConst(message.GetView());
    if(!Reader::Validate(view)) {
        TAU_LOG_WARNING(_log_ctx << "Invalid stun message");
        return;
    }
    switch(HeaderReader::GetType(view)) {
        case kBindingResponse:
            OnStunResponse(view, socket_idx, remote);
            break;
        case kBindingRequest:
            OnStunRequest(std::move(message), view, socket_idx, remote);
            break;
        case kBindingIndication:
            break;
        case kBindingErrorResponse:
            break;
    }
}

State CheckList::GetState() const {
    if(_pairs.empty()) { return State::kWaiting; }
    auto& pair = _pairs.front();
    if(pair.state == CandidatePair::State::kSucceeded)  { return State::kReady; }
    if(pair.state == CandidatePair::State::kNominating) { return State::kReady; }
    if(pair.state == CandidatePair::State::kNominated)  { return State::kCompleted; }
    if(pair.state == CandidatePair::State::kFailed)     { return State::kFailed; }
    return State::kRunning;
}

const CandidatePair& CheckList::GetBestCandidatePair() const {
    return _pairs.front();
}

void CheckList::Nominating() {
    auto& best_pair = _pairs.front();
    if((_role != Role::kControlling) || (best_pair.state != CandidatePair::State::kSucceeded)) {
        return;
    }
    if(_nominating_strategy == NominatingStrategy::kBestValid) {
        for(auto& pair : _pairs) {
            if(pair.state < CandidatePair::State::kSucceeded) {
                return; // some pairs are in-progress yet
            }
        }
    }
    SendStunRequest(*best_pair.local.socket_idx, best_pair.id, best_pair.remote.endpoint, true);
    SetPairState(best_pair.id, CandidatePair::State::kNominating);
    best_pair.attempts_count = 0;
}

void CheckList::ProcessConnectivityChecks(size_t socket_idx) {
    const auto now = _deps.clock.Now();
    Timepoint smallest_last_tp = now;
    size_t pair_id = 0;
    auto& transaction_tracker = _transcation_trackers.at(socket_idx);
    for(auto& pair : _pairs) {
        if(socket_idx == *pair.local.socket_idx) {
            if(pair.state == CandidatePair::State::kWaiting) {
                SetPairState(pair.id, CandidatePair::State::kInProgress);
                SendStunRequest(socket_idx, pair.id, pair.remote.endpoint);
                return;
            }
            if((pair.state == CandidatePair::State::kInProgress) || (pair.state == CandidatePair::State::kNominating)) {
                auto last_request_tp = transaction_tracker.GetLastTimepoint(pair.id);
                if(smallest_last_tp > last_request_tp) {
                    smallest_last_tp = last_request_tp;
                    pair_id = pair.id;
                }
            }
        }
    }
    const size_t max_attempts = (GetState() == State::kRunning) ? 16 : 4;
    if(now >= smallest_last_tp + kRtoDefault) {
        auto& pair = GetPairById(pair_id);
        if(pair.attempts_count < max_attempts) {
            pair.attempts_count++;
            SendStunRequest(socket_idx, pair.id, pair.remote.endpoint, pair.state == CandidatePair::State::kNominating);
        } else {
            SetPairState(pair.id, CandidatePair::State::kFailed);
        }
    }
}

void CheckList::SendStunRequest(size_t socket_idx, size_t pair_id, Endpoint remote, bool nominating) {
    auto stun_request = Buffer::Create(_deps.udp_allocator);
    auto view = stun_request.GetViewWithCapacity();
    stun::Writer writer(view, kBindingRequest);
    auto& transaction_tracker = _transcation_trackers.at(socket_idx);
    transaction_tracker.SetTransactionId(view, pair_id);

    if(nominating) {
        UseCandidateWriter::Write(writer);
    }
    uint64_t tiebreaker = _deps.clock.Now(); //TODO: fix tiebreaker
    IceRoleWriter::Write(writer, (_role == Role::kControlling), tiebreaker);
    DataUint32Writer::Write(writer, AttributeType::kPriority, Priority(CandidateType::kPeerRefl, socket_idx));
    const std::string user_name = _credentials.remote.ufrag + ":" + _credentials.local.ufrag;
    ByteStringWriter::Write(writer, AttributeType::kUserName, user_name);
    MessageIntegrityWriter::Write(writer, _credentials.remote.password);
    FingerprintWriter::Write(writer);
    stun_request.SetSize(writer.GetSize());

    _send_callback(socket_idx, remote, std::move(stun_request));
}

void CheckList::OnStunResponse(const BufferViewConst& view, size_t socket_idx, Endpoint remote) {
    auto& transaction_tracker = _transcation_trackers.at(socket_idx);
    const auto hash = HeaderReader::GetTransactionIdHash(view);
    const auto transaction = transaction_tracker.HasTransaction(hash);
    if(!transaction) {
        TAU_LOG_WARNING(_log_ctx << "Unknown transaction, hash: " << hash);
        return;
    }
    transaction_tracker.RemoveTransaction(hash);
    auto& pair = GetPairById(transaction->tag);
    if(pair.remote.endpoint != remote) {
        TAU_LOG_WARNING(_log_ctx << "Transaction expected remote: " << pair.remote.endpoint << ", actual remote: " << remote);
        SetPairState(pair.id, CandidatePair::State::kFailed);
        return;
    }

    bool nominating = false;
    std::optional<Endpoint> reflexive;
    auto ok = Reader::ForEachAttribute(view, [&](AttributeType type, BufferViewConst attr) {
        switch(type) {
            case AttributeType::kIceControlled:  return (_role == Role::kControlling);
            case AttributeType::kIceControlling: return (_role == Role::kControlled);
            case AttributeType::kMessageIntegrity:
                return MessageIntegrityReader::Validate(attr, view, _credentials.remote.password);
            case AttributeType::kUseCandidate:
                nominating = true;
                break;
            case AttributeType::kXorMappedAddress:
                if(XorMappedAddressReader::GetFamily(attr) == IpFamily::kIpv4) {
                    auto address = XorMappedAddressReader::GetAddressV4(attr);
                    auto port = XorMappedAddressReader::GetPort(attr);
                    reflexive.emplace(Endpoint{IpAddressV4(address), port});
                }
                break;
            default:
                break;
        }
        return true;
    });
    if(ok && reflexive) {
        if(FindCandidateByEndpoint(_local_candidates, *reflexive)) {
            SetPairState(transaction->tag, CandidatePair::State::kSucceeded);
        } else {
            TAU_LOG_INFO(_log_ctx << "Add local peer-reflexive: " << *reflexive << ", socket: " << socket_idx << ", remote: " << remote << ", pair_id: " << transaction->tag);
            AddLocalCandidate(CandidateType::kPeerRefl, socket_idx, *reflexive);
        }
        if(nominating) {
            if(_role == Role::kControlling) {
                SetPairState(transaction->tag, CandidatePair::State::kNominated);
            } else {
                TAU_LOG_WARNING(_log_ctx << "Ignore nomination, wrong role");
            }
        }
    } else {
        TAU_LOG_WARNING(_log_ctx << "Ignore response, ok: " << ok << ", transaction hash: " << HeaderReader::GetTransactionIdHash(view));
    }
}

void CheckList::OnStunRequest(Buffer&& message, const BufferViewConst& view, size_t socket_idx, Endpoint remote) {
    uint32_t priority = 0;
    bool nominating = false;
    bool message_integrity = false;
    auto ok = Reader::ForEachAttribute(view, [&](AttributeType type, BufferViewConst attr) {
        switch(type) {
            case AttributeType::kIceControlled:  return (_role == Role::kControlling);
            case AttributeType::kIceControlling: return (_role == Role::kControlled);
            case AttributeType::kPriority:
                priority = DataUint32Reader::GetValue(attr);
                break;
            // case AttributeType::kUserName: // ignore, message-integrity is used for validation
            case AttributeType::kMessageIntegrity:
                message_integrity = MessageIntegrityReader::Validate(attr, view, _credentials.local.password);
                return message_integrity;
            case AttributeType::kUseCandidate:
                if(_role == Role::kControlled) {
                    nominating = true;
                } else {
                    TAU_LOG_WARNING(_log_ctx << "Ignore nomination with wrong role");
                }
                break;
            default:
                break;
        }
        return true;
    });
    if(ok && message_integrity) {
        if(FindCandidateByEndpoint(_remote_candidates, remote)) {
            if(nominating) {
                nominating = false;
                for(auto& pair : _pairs) {
                    if((*pair.local.socket_idx == socket_idx) && (pair.remote.endpoint == remote)) {
                        if((pair.state == CandidatePair::State::kSucceeded) || (pair.state == CandidatePair::State::kNominated)) {
                            SetPairState(pair.id, CandidatePair::State::kNominated);
                            nominating = true;
                            break;
                        }
                    }
                }
            }
        } else {
            TAU_LOG_INFO(_log_ctx << "Add peer-reflexive remote candidate: " << remote << ", priority: " << priority << ", socket_idx: " << socket_idx);
            _remote_candidates.push_back(Candidate{
                .type = CandidateType::kPeerRefl,
                .priority = priority,
                .endpoint = remote
            });
            for(auto& local : _local_candidates) {
                if(*local.socket_idx == socket_idx) {
                    AddPair(local, _remote_candidates.back());
                }
            }
            nominating = false; // send response to notify about new peer-reflexive address using XorMappedAddress
        }

        // prepare response and send
        stun::Writer writer(message.GetViewWithCapacity(), kBindingResponse);
        if(nominating) {
            UseCandidateWriter::Write(writer);
        }
        XorMappedAddressWriter::Write(writer, AttributeType::kXorMappedAddress,
            remote.address().to_v4().to_uint(), remote.port());
        MessageIntegrityWriter::Write(writer, _credentials.local.password);
        FingerprintWriter::Write(writer);
        message.SetSize(writer.GetSize());

        _send_callback(socket_idx, remote, std::move(message));
    } else {
        TAU_LOG_WARNING(_log_ctx << "Ignore malformed message, transaction hash: " << HeaderReader::GetTransactionIdHash(view));
    }
}

// https://www.rfc-editor.org/rfc/rfc8445.html#section-6.1.2.4
void CheckList::PrunePairs() {
    for(auto it = _pairs.begin(); it != _pairs.end(); ++it) {
        for(auto it2 = std::next(it); it2 != _pairs.end(); ) {
            const auto same_socket = (*it->local.socket_idx == *it2->local.socket_idx);
            const auto same_remote = (it->remote.endpoint == it2->remote.endpoint);
            if(same_socket && same_remote) {
                it2 = _pairs.erase(it2);
            } else {
                it2++;
            }
        }
    }
}

void CheckList::AddPair(const Candidate& local, const Candidate& remote) {
    _pairs.emplace_back(CandidatePair{
        .id = _next_pair_id++,
        .local = local,
        .remote = remote,
        .priority = PairPriority(_role, local.priority, remote.priority),
        .state = CandidatePair::State::kWaiting
    });
    std::sort(_pairs.begin(), _pairs.end());
}

bool CheckList::SetPairState(size_t id, CandidatePair::State state) {
    auto& pair = GetPairById(id);
    if(pair.state < state) {
        pair.state = state;
        std::sort(_pairs.begin(), _pairs.end());
        return true;
    }
    return false;
}

CandidatePair& CheckList::GetPairById(size_t id) {
    for(auto& pair : _pairs) {
        if(pair.id == id) {
            return pair;
        }
    }
    assert(false && "Can't find pair id");
    return _pairs.front();
}

std::optional<size_t> CheckList::FindCandidateByEndpoint(const Candidates& candidates, Endpoint endpoint) {
    for(size_t i = 0; i < candidates.size(); ++i) {
        if(candidates[i].endpoint == endpoint) {
            return i;
        }
    }
    return std::nullopt;
}

}
