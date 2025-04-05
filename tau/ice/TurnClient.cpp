#include "tau/ice/TurnClient.h"
#include "tau/stun/Reader.h"
#include "tau/stun/Writer.h"
#include "tau/stun/attribute/XorMappedAddress.h"
#include "tau/stun/attribute/DataUint32.h"
#include "tau/stun/attribute/ByteString.h"
#include "tau/stun/attribute/Data.h"
#include "tau/stun/attribute/MessageIntegrity.h"
#include "tau/stun/attribute/Fingerprint.h"
#include "tau/crypto/Md5.h"
#include "tau/common/Container.h"
#include "tau/common/Log.h"
#include <cassert>

namespace tau::ice {

using namespace stun;
using namespace stun::attribute;

TurnClient::TurnClient(Dependencies&& deps, Options&& options)
    : _deps(std::move(deps))
    , _options(std::move(options))
    , _next_request_tp(_deps.clock.Now() + _options.start_delay)
    , _transaction_tracker(_deps.clock)
    , _allocation_eol(_deps.clock.Now())
{}

void TurnClient::Process() {
    if(_deps.clock.Now() < _next_request_tp) {
        ProcessPermissionsRto();
        return;
    }

    if(!_relayed) {
        SendAllocationRequest(!_realm.empty());
    } else {
        SendRefreshRequest();
    }
}

void TurnClient::Recv(Buffer&& message) {
    auto view = ToConst(message.GetView());
    if(!Reader::Validate(view)) {
        LOG_WARNING << "Invalid stun message";
        return;
    }

    const auto type = HeaderReader::GetType(view);
    const auto hash = HeaderReader::GetTransactionIdHash(view);
    switch(type) {
        case kCreatePermissionResponse: return OnCreatePermissionResponse(hash);
        case kDataIndication:           return OnDataIndication(std::move(message));
        default:
            if((hash != _transaction_hash) && !_transaction_tracker.HasTransaction(hash)) {
                LOG_WARNING << "Unknown transaction, hash: " << hash;
                return;
            }
            break;
    }
    _transaction_tracker.RemoveTransaction(hash);
    OnStunResponse(view);
}

void TurnClient::Send(Buffer&& packet, Endpoint remote) {
    if(!HasPermission(remote.address())) {
        LOG_WARNING << "No permission, remote: " << remote;
        return;
    }

    auto indication = Buffer::Create(_deps.udp_allocator);
    auto view = indication.GetViewWithCapacity();
    stun::Writer writer(view, kSendIndication);
    auto transaction_id_ptr = view.ptr + 2 * sizeof(uint32_t);
    stun::GenerateTransactionId(transaction_id_ptr);

    XorMappedAddressWriter::Write(writer, AttributeType::kXorPeerAddress,
        remote.address().to_v4().to_uint(), remote.port());
    //TODO: DONT-FRAGMENT attribute
    DataWriter::Write(writer, ToConst(packet.GetView()));
    indication.SetSize(writer.GetSize());

    _send_callback(_options.server, std::move(indication));
}

void TurnClient::CreatePermission(asio_ip::address remote) {
    if(!Contains(_permissions, remote)) {
        _permissions.insert(std::make_pair(remote, Permission{
            .done = false,
            .rto_tp = _deps.clock.Now() + kRtoDefault
        }));
        SendCreatePermissionRequest(remote);
    }
}

bool TurnClient::HasPermission(asio_ip::address remote) {
    auto it = _permissions.find(remote);
    if(it != _permissions.end()) {
        return it->second.done;
    }
    return false;
}

void TurnClient::Stop() {
    SendRefreshRequest(0);
}

bool TurnClient::IsServerEndpoint(Endpoint remote) const {
    return (remote == _options.server);
}

void TurnClient::ProcessPermissionsRto() {
    const auto now = _deps.clock.Now();
    for(auto& [remote, permission] : _permissions) {
        if(!permission.done) {
            if(now >= permission.rto_tp) {
                permission.rto_tp = now + kRtoDefault;
                SendCreatePermissionRequest(remote);
                break;
            }
        }
    }
}

void TurnClient::SendAllocationRequest(bool authenticated) {
    auto request = Buffer::Create(_deps.udp_allocator);
    auto view = request.GetViewWithCapacity();
    stun::Writer writer(view, kAllocateRequest);
    if(!authenticated) {
        _transaction_id.resize(kTransactionIdSize);
        _transaction_hash = GenerateTransactionId(_transaction_id.data());
    }
    std::memcpy(view.ptr + 2 * sizeof(uint32_t), _transaction_id.data(), _transaction_id.size());

    if(authenticated) {
        DataUint32Writer::Write(writer, AttributeType::kRequestedTransport, 0x11 << 24);
        ByteStringWriter::Write(writer, AttributeType::kUserName, _options.credentials.ufrag);
        ByteStringWriter::Write(writer, AttributeType::kRealm, _realm);
        ByteStringWriter::Write(writer, AttributeType::kNonce, _nonce);

        std::array<uint8_t, crypto::kMd5DigestLength> hash;
        if(!CalcLongTermPassword(_options.credentials, _realm, hash.data())) {
            return;
        }
        MessageIntegrityWriter::Write(writer, std::string_view{reinterpret_cast<char*>(hash.data()), hash.size()});
        FingerprintWriter::Write(writer);
    }
    request.SetSize(writer.GetSize());
    _next_request_tp = _deps.clock.Now() + kRtoDefault;

    _send_callback(_options.server, std::move(request));
}

void TurnClient::SendRefreshRequest(size_t refresh_sec) {
    auto request = Buffer::Create(_deps.udp_allocator);
    auto view = request.GetViewWithCapacity();
    stun::Writer writer(view, kRefreshRequest);
    _transaction_tracker.SetTransactionId(view, 0);

    DataUint32Writer::Write(writer, AttributeType::kLifetime, refresh_sec);
    ByteStringWriter::Write(writer, AttributeType::kUserName, _options.credentials.ufrag);
    ByteStringWriter::Write(writer, AttributeType::kRealm, _realm);
    ByteStringWriter::Write(writer, AttributeType::kNonce, _nonce);
    MessageIntegrityWriter::Write(writer, _message_integrity_password);
    FingerprintWriter::Write(writer);

    request.SetSize(writer.GetSize());
    _next_request_tp = _deps.clock.Now() + kRtoDefault;

    _send_callback(_options.server, std::move(request));
}

void TurnClient::SendCreatePermissionRequest(asio_ip::address remote) {
    auto request = Buffer::Create(_deps.udp_allocator);
    auto view = request.GetViewWithCapacity();
    stun::Writer writer(view, kCreatePermissionRequest);

    uint32_t ip4_addr = remote.to_v4().to_uint();
    _transaction_tracker.SetTransactionId(view, ip4_addr);

    XorMappedAddressWriter::Write(writer, AttributeType::kXorPeerAddress, ip4_addr, 0);
    ByteStringWriter::Write(writer, AttributeType::kUserName, _options.credentials.ufrag);
    ByteStringWriter::Write(writer, AttributeType::kRealm, _realm);
    ByteStringWriter::Write(writer, AttributeType::kNonce, _nonce);
    MessageIntegrityWriter::Write(writer, _message_integrity_password);
    FingerprintWriter::Write(writer);

    request.SetSize(writer.GetSize());
    _next_request_tp = _deps.clock.Now() + kRtoDefault;

    _send_callback(_options.server, std::move(request));
}

void TurnClient::OnStunResponse(const BufferViewConst& view) {
    auto ok = Reader::ForEachAttribute(view, [&, this](AttributeType type, BufferViewConst attr) {
        switch(type) {
            case AttributeType::kXorRelayedAddress:
                if(XorMappedAddressReader::GetFamily(attr) == IpFamily::kIpv4) {
                    auto address = XorMappedAddressReader::GetAddressV4(attr);
                    auto port = XorMappedAddressReader::GetPort(attr);
                    _relayed.emplace(Endpoint{asio_ip::address_v4(address), port});
                }
                break;
            case AttributeType::kRealm:
                _realm = ByteStringReader::GetValue(attr);
                UpdateMessageIntegrityPassword();
                break;
            case AttributeType::kNonce:
                _nonce = ByteStringReader::GetValue(attr);
                UpdateMessageIntegrityPassword();
                break;
            case AttributeType::kLifetime:
                _allocation_eol = _deps.clock.Now() + kSec * DataUint32Reader::GetValue(attr);
                break;
            case AttributeType::kMessageIntegrity:
                return MessageIntegrityReader::Validate(attr, view, _message_integrity_password);
            default:
                break;
        }
        return true;
    });
    if(ok) {
        const auto now = _deps.clock.Now();
        if(_relayed && (now < _allocation_eol)) {
            _next_request_tp = std::min(now + kMin, _allocation_eol - 10 * kSec);
            _candidate_callback(CandidateType::kServRefl, *_relayed);
        } else {
            _next_request_tp = now + kTaDefault;
        }
    } else {
        _relayed.reset();
    }
}

void TurnClient::OnCreatePermissionResponse(uint32_t hash) {
    auto result = _transaction_tracker.HasTransaction(hash);
    if(!result) {
        LOG_WARNING << "Unknown hash: " << hash;
        return;
    }

    for(auto& [remote, permission] : _permissions) {
        auto ip4_addr = remote.to_v4().to_uint();
        if(ip4_addr == result->tag) {
            permission.done = true;
            break;
        }
    }
    _transaction_tracker.RemoveTransaction(hash);
}

void TurnClient::OnDataIndication(Buffer&& message) {
    std::optional<Endpoint> remote_peer;
    std::optional<BufferViewConst> data;
    auto view = message.GetView();
    auto ok = Reader::ForEachAttribute(ToConst(view), [&](AttributeType type, BufferViewConst attr) {
        switch(type) {
            case AttributeType::kXorPeerAddress:
                if(XorMappedAddressReader::GetFamily(attr) == IpFamily::kIpv4) {
                    auto address = XorMappedAddressReader::GetAddressV4(attr);
                    auto port = XorMappedAddressReader::GetPort(attr);
                    remote_peer.emplace(Endpoint{asio_ip::address_v4(address), port});
                }
                break;
            case AttributeType::kData:
                data.emplace(DataReader::GetData(attr));
                break;
            default:
                break;
        }
        return true;
    });
    if(!ok || !remote_peer || !data || !data->size) {
        LOG_WARNING << "Wrong data indication";
        return;
    }

    std::memmove(view.ptr, data->ptr, data->size);
    message.SetSize(data->size);

    _recv_callback(*remote_peer, std::move(message));
}

void TurnClient::UpdateMessageIntegrityPassword() {
    _message_integrity_password.resize(kLongTermPassword);
    auto password_ptr = reinterpret_cast<uint8_t*>(_message_integrity_password.data());
    if(!CalcLongTermPassword(_options.credentials, _realm, password_ptr)) {
        LOG_WARNING << "CalcLongTermPassword failed";
    }
}

}
