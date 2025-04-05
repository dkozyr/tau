#include "tests/ice/TurnServerEmulator.h"
#include "tau/ice/Credentials.h"
#include "tau/stun/Reader.h"
#include "tau/stun/attribute/XorMappedAddress.h"
#include "tau/stun/attribute/DataUint32.h"
#include "tau/stun/attribute/ByteString.h"
#include "tau/stun/attribute/Data.h"
#include "tau/stun/attribute/MessageIntegrity.h"
#include "tau/stun/attribute/Fingerprint.h"
#include "tau/crypto/Md5.h"

namespace tau::ice {

using namespace stun;
using namespace stun::attribute;

TurnServerEmulator::TurnServerEmulator(Clock& clock, Options&& options)
    : _clock(clock)
    , _options(std::move(options))
{}

void TurnServerEmulator::Recv(Buffer&& packet, Endpoint src) {
    auto view = ToConst(packet.GetView());
    if(!Reader::Validate(view)) {
        return DropPacket("Invalid stun message");
    }

    const auto type = HeaderReader::GetType(view);
    const auto hash = HeaderReader::GetTransactionIdHash(view);
    switch(type) {
        case kAllocateRequest:
            OnAllocateRequest(std::move(packet), src, hash);
            break;
        case kRefreshRequest:
            OnRefreshRequest(std::move(packet), src);
            break;
        case kCreatePermissionRequest:
            OnCreatePermissionRequest(std::move(packet), src);
            break;
        case kSendIndication:
            OnSendIndication(std::move(packet), src);
            break;
        default:
            DropPacket("Unknown type: " + std::to_string(type));
            break;
    }
}

size_t TurnServerEmulator::GetDroppedPacketsCount() const { return _dropped_packets_count; }

void TurnServerEmulator::OnAllocateRequest(Buffer&& message, Endpoint src, uint32_t hash) {
    std::string user_name;
    std::string realm;
    std::string nonce;
    bool requested_transport = false;
    bool message_integrity = false;
    auto view = ToConst(message.GetView());
    auto ok = Reader::ForEachAttribute(view, [&](AttributeType type, BufferViewConst attr) {
        switch(type) {
            case AttributeType::kRequestedTransport:
                requested_transport = (attribute::DataUint32Reader::GetValue(attr) == (0x11 << 24));
                return requested_transport;
            case AttributeType::kUserName:
                user_name = ByteStringReader::GetValue(attr);
                break;
            case AttributeType::kRealm:
                realm = ByteStringReader::GetValue(attr);
                break;
            case AttributeType::kNonce:
                nonce = ByteStringReader::GetValue(attr);
                break;
            case AttributeType::kMessageIntegrity: {
                std::string message_integrity_password(kLongTermPassword, 'x');
                auto password_ptr = reinterpret_cast<uint8_t*>(message_integrity_password.data());
                if(!CalcLongTermPassword({user_name, _options.password}, realm, password_ptr)) {
                    LOG_WARNING << "CalcLongTermPassword failed";
                    return false;
                }
                message_integrity = MessageIntegrityReader::Validate(attr, view, message_integrity_password);
                return message_integrity;
            }
            default:
                break;
        }
        return true;
    });
    if(!ok) {
        return DropPacket("stun message reader failed");
    }

    if(user_name.empty() && realm.empty() && nonce.empty()) {
        OnAllocateRequestInitial(std::move(message), src, hash);
    } else {
        if(Contains(_hash_to_nonce, hash)) {
            if(nonce != _hash_to_nonce.at(hash)) {
                return DropPacket("Wrong nonce: " + nonce);
            }
            if(realm != _realm) {
                return DropPacket("Wrong realm: " + realm);
            }
            if(!requested_transport) {
                return DropPacket("No requested transport");
            }
            if(!message_integrity) {
                return DropPacket("Wrong message_integrity");
            }
            _hash_to_nonce.erase(hash);
            OnAllocateRequest(std::move(message), src, user_name, nonce);
        } else {
            return DropPacket("Wrong transaction id");
        }
    }
}

void TurnServerEmulator::OnAllocateRequestInitial(Buffer&& message, Endpoint src, uint32_t hash) {
    auto view = message.GetViewWithCapacity();
    stun::Writer writer(view);
    writer.WriteHeader(kAllocateErrorResponse);

    auto nonce = ToHexString(g_random.Int<uint64_t>());
    _hash_to_nonce[hash] = nonce;

    //TODO: ERROR attribute
    ByteStringWriter::Write(writer, AttributeType::kRealm, _realm);
    ByteStringWriter::Write(writer, AttributeType::kNonce, nonce);
    message.SetSize(writer.GetSize());

    _on_send_callback(std::move(message), kEndpointDefault, src);
}

void TurnServerEmulator::OnAllocateRequest(Buffer&& message, Endpoint src, const std::string& user_name, const std::string& nonce) {
    auto view = message.GetViewWithCapacity();
    stun::Writer writer(view);
    writer.WriteHeader(kAllocateResponse);

    XorMappedAddressWriter::Write(writer, AttributeType::kXorRelayedAddress,
        _options.public_ip.to_v4().to_uint(), _latest_port);
    XorMappedAddressWriter::Write(writer, AttributeType::kXorMappedAddress,
        src.address().to_v4().to_uint(), src.port());
    DataUint32Writer::Write(writer, AttributeType::kLifetime, 600);
    FinalizeStunMessage(message, writer, user_name);

    _public_to_remote[src] = Allocation{
        .user_name = user_name,
        .nonce = nonce,
        .port = _latest_port,
        .expire_time = _clock.Now() + 600 * kSec
    };
    _latest_port++;

    _on_send_callback(std::move(message), kEndpointDefault, src);
}

void TurnServerEmulator::OnRefreshRequest(Buffer&& message, Endpoint src) {
    if(!Contains(_public_to_remote, src)) {
        return DropPacket("Refresh request from unknown endpoint");
    }
    auto view = message.GetViewWithCapacity();
    stun::Writer writer(view);
    writer.WriteHeader(kRefreshResponse);

    XorMappedAddressWriter::Write(writer, AttributeType::kXorRelayedAddress,
        _options.public_ip.to_v4().to_uint(), _latest_port);
    XorMappedAddressWriter::Write(writer, AttributeType::kXorMappedAddress,
        src.address().to_v4().to_uint(), src.port());
    DataUint32Writer::Write(writer, AttributeType::kLifetime, 600);
    FinalizeStunMessage(message, writer, _public_to_remote.at(src).user_name);

    _on_send_callback(std::move(message), kEndpointDefault, src);
}

void TurnServerEmulator::OnCreatePermissionRequest(Buffer&& message, Endpoint src) {
    auto it = _public_to_remote.find(src);
    if(it == _public_to_remote.end()) {
        return DropPacket("Create permission request from unknown endpoint");
    }
    auto& allocation = it->second;

    bool peer_address = false;
    bool message_integrity = false;
    auto request_view = ToConst(message.GetView());
    auto ok = Reader::ForEachAttribute(request_view, [&](AttributeType type, BufferViewConst attr) {
        switch(type) {
            case AttributeType::kXorPeerAddress:
                if(XorMappedAddressReader::GetFamily(attr) == IpFamily::kIpv4) {
                    auto address = XorMappedAddressReader::GetAddressV4(attr);
                    allocation.permissions.insert(asio_ip::address_v4(address));
                    peer_address = true;
                }
                break;
            case AttributeType::kUserName: return (allocation.user_name == ByteStringReader::GetValue(attr));
            case AttributeType::kRealm:    return (_realm == ByteStringReader::GetValue(attr));
            case AttributeType::kNonce:    return (allocation.nonce == ByteStringReader::GetValue(attr));
            case AttributeType::kMessageIntegrity: {
                std::string message_integrity_password(kLongTermPassword, 'x');
                auto password_ptr = reinterpret_cast<uint8_t*>(message_integrity_password.data());
                if(!CalcLongTermPassword({allocation.user_name, _options.password}, _realm, password_ptr)) {
                    LOG_WARNING << "CalcLongTermPassword failed";
                    return false;
                }
                message_integrity = MessageIntegrityReader::Validate(attr, request_view, message_integrity_password);
                return message_integrity;
            }
            default:
                break;
        }
        return true;
    });
    if(!ok || !message_integrity || !peer_address) {
        return DropPacket("stun message reader failed");
    }

    stun::Writer writer(message.GetViewWithCapacity());
    writer.WriteHeader(kCreatePermissionResponse);
    FinalizeStunMessage(message, writer, _public_to_remote.at(src).user_name);

    _on_send_callback(std::move(message), kEndpointDefault, src);
}

void TurnServerEmulator::OnSendIndication(Buffer&& message, Endpoint src) {
    auto it = _public_to_remote.find(src);
    if(it == _public_to_remote.end()) {
        return DropPacket("Create permission request from unknown endpoint");
    }
    auto& allocation = it->second;

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
        return DropPacket("Wrong send indication");
    }
    if(!Contains(allocation.permissions, remote_peer->address())) {
        return DropPacket("No permission for peer: " + ToString(*remote_peer));
    }

    std::memmove(view.ptr, data->ptr, data->size);
    message.SetSize(data->size);

    _on_send_callback(std::move(message), kEndpointDefault, *remote_peer);
}

void TurnServerEmulator::FinalizeStunMessage(Buffer& message, stun::Writer& writer, const std::string& user_name) {
    std::array<uint8_t, crypto::kMd5DigestLength> hash;
    if(!CalcLongTermPassword({user_name, _options.password}, _realm, hash.data())) {
        return DropPacket("CalcLongTermPassword failed");
    }
    MessageIntegrityWriter::Write(writer, std::string_view{reinterpret_cast<char*>(hash.data()), hash.size()});
    FingerprintWriter::Write(writer);
    message.SetSize(writer.GetSize());
}

void TurnServerEmulator::DropPacket(const std::string& message) {
    LOG_WARNING << message << ", drop packet";
    _dropped_packets_count++;
}

}
