#include "tau/ice/CheckList.h"
#include "tau/stun/Reader.h"
#include "tau/stun/Writer.h"
#include "tau/stun/attribute/XorMappedAddress.h"
#include "tau/stun/attribute/Priority.h"
#include "tau/stun/attribute/UseCandidate.h"
#include "tau/stun/attribute/IceRole.h"
#include "tau/stun/attribute/UserName.h"
#include "tau/stun/attribute/MessageIntegrity.h"
#include "tau/stun/attribute/Fingerprint.h"
#include "tau/sdp/line/attribute/Candidate.h"
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
    , _log_ctx(std::move(options.log_ctx))
    , _sockets(std::move(options.sockets))
    , _stun_server(std::move(options.stun_server))
    , _last_ta_tp(_deps.clock.Now() - kTaDefault) {
    for(size_t i = 0; i < _sockets.size(); ++i) {
        _transcation_trackers.insert({i, TransactionTracker(_deps.clock)});
    }
}

CheckList::~CheckList() {
    LOG_DEBUG << _log_ctx << "Candidate pairs:";
    for(auto& pair : _pairs) {
        LOG_DEBUG << _log_ctx << "id: " << pair.id << ", local: " << (size_t)pair.local.type << ", remote: " << (size_t)pair.remote.type
            << ", priority: " << pair.priority << ", socket: " << pair.local.socket_idx.value()
            << ", remote: " << pair.remote.endpoint << ", state: " << pair.state << ", attempts: " << pair.attempts_count;
    }
}

void CheckList::Start() {
    for(size_t i = 0; i < _sockets.size(); ++i) {
        auto& socket = _sockets[i];
        ProcessLocalCandidate(CandidateType::kHost, i, socket);
        SendStunRequest(i, std::nullopt, _stun_server, false);
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

    ProcessConnectivityChecks();
}

void CheckList::ProcessConnectivityChecks() {
    for(size_t socket_idx = 0; socket_idx < _sockets.size(); ++socket_idx) {
        for(auto& pair : _pairs) {
            if(socket_idx == *pair.local.socket_idx) {
                if(pair.state == CandidatePair::State::kWaiting) {
                    SendStunRequest(socket_idx, pair.id, pair.remote.endpoint);
                    pair.state = CandidatePair::State::kInProgress;
                    break;
                }
                if((pair.state == CandidatePair::State::kSucceeded) && (_role == Role::kControlling)) {
                    //TODO: there shouldn't be in-progress pairs on nominating
                    if(_pairs.front().id == pair.id) {
                        SendStunRequest(socket_idx, pair.id, pair.remote.endpoint, true, true);
                        pair.state = CandidatePair::State::kNominating;
                        pair.attempts_count = 0;
                        break;
                    }
                }
            }
        }

        auto& transaction_tracker = _transcation_trackers.at(socket_idx);
        const auto now = _deps.clock.Now();
        Timepoint smallest_last_tp = now;
        size_t pair_id = 0;
        for(auto& pair : _pairs) {
            if(socket_idx == *pair.local.socket_idx) {
                if((pair.state == CandidatePair::State::kInProgress) || (pair.state == CandidatePair::State::kNominating)) {
                    auto last_request_tp = transaction_tracker.GetLastTimepoint(pair.id);
                    if(smallest_last_tp > last_request_tp) {
                        smallest_last_tp = last_request_tp;
                        pair_id = pair.id;
                    }
                }
            }
        }
        if(now >= smallest_last_tp + kRtoDefault) {
            auto& pair = GetPairById(pair_id);
            if(pair.attempts_count < 8) {
                pair.attempts_count++;
                SendStunRequest(socket_idx, pair.id, pair.remote.endpoint, true, pair.state == CandidatePair::State::kNominating);
            } else {
                pair.state = CandidatePair::State::kFailed;
            }
        }
    }
}

void CheckList::RecvRemoteCandidate(std::string candidate) {
    ToLowerCase(candidate);
    LOG_INFO << _log_ctx << "Candidate: " << candidate;
    if(!sdp::attribute::CandidateReader::Validate(candidate)) {
        LOG_WARNING << "Invalid remote candidate: " << candidate;
        return;
    }
    if(sdp::attribute::CandidateReader::GetTransport(candidate) != "udp") {
        return;
    }
    if(sdp::attribute::CandidateReader::GetComponentId(candidate) != 1) {
        return;
    }
    auto address = sdp::attribute::CandidateReader::GetAddress(candidate);
    auto port = sdp::attribute::CandidateReader::GetPort(candidate);
    Endpoint endpoint{asio_ip::make_address(address), port};
    if(!endpoint.address().is_v4()) {
        return;
    }
    _remote_candidates.push_back(Candidate{
        .type = CandidateTypeFromString(sdp::attribute::CandidateReader::GetType(candidate)),
        .priority = sdp::attribute::CandidateReader::GetPriority(candidate),
        .endpoint = endpoint
    });
    for(auto& local : _local_candidates) {
        AddPair(local, _remote_candidates.back());
    }
    PrunePairs();
}

void CheckList::Recv(Endpoint local, Endpoint remote, Buffer&& message) {
    auto view = ToConst(message.GetView());
    if(!Reader::Validate(view)) {
        LOG_WARNING << "Invalid stun message";
        return;
    }

    const auto socket_idx = GetSocketIdxByEndpoint(local); //TODO: can we remove it?
    switch(HeaderReader::GetType(view)) {
        case BindingType::kResponse:
            OnStunResponse(view, socket_idx, remote);
            break;
        case BindingType::kRequest:
            OnStunRequest(std::move(message), view, socket_idx, remote);
            break;
        case BindingType::kIndication:
            break;
        case BindingType::kErrorResponse:
            break;
    }
}

CheckList::State CheckList::GetState() const {
    if(_pairs.empty()) { return State::kWaiting; }
    for(auto& pair : _pairs) {
        if(pair.state == CandidatePair::State::kFrozen)     { return State::kRunning; }
        if(pair.state == CandidatePair::State::kWaiting)    { return State::kRunning; }
        if(pair.state == CandidatePair::State::kInProgress) { return State::kRunning; }
    }
    auto& pair = _pairs.front();
    if(pair.state == CandidatePair::State::kFailed)    { return State::kFailed; }
    if(pair.state == CandidatePair::State::kNominated) { return State::kCompleted; }
    return State::kRunning;
}

void CheckList::ProcessLocalCandidate(CandidateType type, size_t socket_idx, Endpoint endpoint) {
    _local_candidates.push_back(Candidate{
        .type = type,
        .priority = Priority(type, socket_idx),
        .endpoint = endpoint,
        .socket_idx = socket_idx
    });
    _candidate_callback(ToCandidateAttributeString(type, socket_idx, endpoint));
}

void CheckList::SendStunRequest(size_t socket_idx, std::optional<size_t> pair_id, Endpoint remote, bool authenticated, bool nominating) {
    auto stun_request = Buffer::Create(_deps.udp_allocator);
    auto view = stun_request.GetViewWithCapacity();
    Writer writer(view);
    writer.WriteHeader(BindingType::kRequest);
    auto& transaction_tracker = _transcation_trackers.at(socket_idx);
    transaction_tracker.SetTransactionId(view, pair_id);

    if(authenticated) {
        if(nominating) {
            UseCandidateWriter::Write(writer);
        }
        uint64_t tiebreaker = _deps.clock.Now(); //TODO: fix tiebreaker
        IceRoleWriter::Write(writer, (_role == Role::kControlling), tiebreaker);
        PriorityWriter::Write(writer, Priority(CandidateType::kPeerRefl, socket_idx));
        UserNameWriter::Write(writer, _credentials.local.ufrag, _credentials.remote.ufrag);
        MessageIntegrityWriter::Write(writer, _credentials.remote.password);
        FingerprintWriter::Write(writer);
    }
    stun_request.SetSize(writer.GetSize());

    _send_callback(_sockets[socket_idx], remote, std::move(stun_request));
}

void CheckList::OnStunResponse(const BufferViewConst& view, size_t socket_idx, Endpoint remote) {
    auto& transaction_tracker = _transcation_trackers.at(socket_idx);
    const auto hash = HeaderReader::GetTransactionIdHash(view);
    const auto transaction = transaction_tracker.HasTransaction(hash);
    if(!transaction) {
        LOG_WARNING << "Unknown transaction, hash: " << hash;
        return;
    }
    if(transaction->pair_id) {
        auto& pair = GetPairById(*transaction->pair_id);
        if(pair.remote.endpoint != remote) {
            LOG_WARNING << "Transaction expected remote: " << pair.remote.endpoint << ", actual remote: " << remote;
            pair.state = CandidatePair::State::kFailed;
            return;
        }
    } else if(remote != _stun_server) {
        LOG_WARNING << "Unexpected remote endpoint: " << remote;
        return;
    }

    bool nominating = false;
    std::optional<Endpoint> reflexive;
    auto ok = Reader::ForEachAttribute(view, [&](AttributeType type, BufferViewConst attr) {
        switch(type) {
            case AttributeType::kIceControlled:  return (_role == Role::kControlling);
            case AttributeType::kIceControlling: return (_role == Role::kControlled);
            case AttributeType::kMessageIntegrity:
                return MessageIntegrityReader::Validate(attr, view, _credentials.local.password);
            case AttributeType::kUseCandidate:
                nominating = true;
                break;
            case AttributeType::kXorMappedAddress:
                if(XorMappedAddressReader::GetFamily(attr) == IpFamily::kIpv4) {
                    auto address = XorMappedAddressReader::GetAddressV4(attr);
                    auto port = XorMappedAddressReader::GetPort(attr);
                    reflexive.emplace(Endpoint{asio_ip::address_v4(address), port});
                }
                break;
            default:
                break;
        }
        return true;
    });
    if(ok && reflexive) {
        if(remote == _stun_server) {
            ProcessLocalCandidate(CandidateType::kServRefl, socket_idx, *reflexive);
        } else {
            if(FindCandidateByEndpoint(_local_candidates, *reflexive)) {
                //TODO: we have a valid pair, ICE agent is ready for transceiving
                SetPairState(*transaction->pair_id, CandidatePair::State::kSucceeded);
            } else {
                SetPairState(*transaction->pair_id, CandidatePair::State::kFailed);

                LOG_INFO << _log_ctx <<"Add local peer-reflexive: " << *reflexive << ", socket: " << socket_idx << ", remote: " << remote;
                _local_candidates.push_back(Candidate{
                    .type = CandidateType::kPeerRefl,
                    .priority = Priority(CandidateType::kPeerRefl, socket_idx),
                    .endpoint = *reflexive,
                    .socket_idx = socket_idx
                });
                if(auto idx = FindCandidateByEndpoint(_remote_candidates, remote)) {
                    AddPair(_local_candidates.back(), _remote_candidates.at(*idx));
                };
            }
        }
        if(nominating) {
            if(_role == Role::kControlling) {
                SetPairState(*transaction->pair_id, CandidatePair::State::kNominated);
            } else {
                LOG_WARNING << _log_ctx << "Ignore nomination, wrong role";
            }
        }
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
                priority = PriorityReader::GetPriority(attr);
                break;
            case AttributeType::kMessageIntegrity:
                message_integrity = MessageIntegrityReader::Validate(attr, view, _credentials.local.password);
                return message_integrity;
            case AttributeType::kUseCandidate:
                if(_role == Role::kControlled) {
                    nominating = true;
                } else {
                    LOG_WARNING << _log_ctx << "Ignore nomination with wrong role";
                }
                break;
            // case AttributeType::kUserName: // ignore, message-integrity is used for validation
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
                            pair.state = CandidatePair::State::kNominated;
                            nominating = true;
                            break;
                        }
                    }
                }
            }
        } else {
            LOG_INFO << _log_ctx << "Add peer-reflexive remote candidate: " << remote << ", priority: " << priority;
            _remote_candidates.push_back(Candidate{
                .type = CandidateType::kPeerRefl,
                .priority = priority,
                .endpoint = remote
            });
            AddPair(_local_candidates.at(socket_idx), _remote_candidates.back());
            nominating = false;
            // return; // send response to notify about new peer-reflexive address using XorMappedAddress
        }

        // prepare response and send
        Writer writer(message.GetViewWithCapacity());
        writer.WriteHeader(BindingType::kResponse);
        if(nominating) {
            UseCandidateWriter::Write(writer);
        }
        XorMappedAddressWriter::Write(writer, remote.address().to_v4().to_uint(), remote.port());
        MessageIntegrityWriter::Write(writer, _credentials.remote.password);
        FingerprintWriter::Write(writer);
        message.SetSize(writer.GetSize());

        _send_callback(_sockets[socket_idx], remote, std::move(message));
    } else {
        LOG_WARNING << "Ignore malformed message, transcation hash: " << HeaderReader::GetTransactionIdHash(view);
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
    _pairs.push_back(CandidatePair{
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

size_t CheckList::GetSocketIdxByEndpoint(Endpoint local) const {
    for(size_t i = 0; i < _sockets.size(); ++i) {
        if(_sockets[i] == local) {
            return i;
        }
    }
    assert(false && "Unknown local endpoint");
    return 0;
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
