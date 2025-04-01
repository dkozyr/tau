#include "tau/ice/StunClient.h"
#include "tau/ice/Constants.h"
#include "tau/stun/Reader.h"
#include "tau/stun/Writer.h"
#include "tau/stun/attribute/XorMappedAddress.h"
#include "tau/common/Log.h"

namespace tau::ice {

using namespace stun;
using namespace stun::attribute;

StunClient::StunClient(Dependencies&& deps, Endpoint server)
    : _deps(std::move(deps))
    , _server(server)
    , _transaction_tracker(_deps.clock)
{}

void StunClient::Process() {
    const auto now = _deps.clock.Now();
    auto tp = _transaction_tracker.GetLastTimepoint(0);
    if(now >= tp + WaitPeriodToNextRequest()) {
        SendStunRequest();
    }
}

void StunClient::Recv(Buffer&& message) {
    auto view = ToConst(message.GetView());
    if(!Reader::Validate(view)) {
        LOG_WARNING << "Invalid stun message";
        return;
    }

    const auto hash = HeaderReader::GetTransactionIdHash(view);
    if(!_transaction_tracker.HasTransaction(hash)) {
        LOG_WARNING << "Unknown transaction, hash: " << hash;
        return;
    }
    _transaction_tracker.RemoveTransaction(hash);

    if(HeaderReader::GetType(view) == BindingType::kResponse) {
        OnStunResponse(view);
    }
}

bool StunClient::IsServerEndpoint(Endpoint remote) const {
    return (remote == _server);
}

void StunClient::SendStunRequest() {
    auto stun_request = Buffer::Create(_deps.udp_allocator);
    auto view = stun_request.GetViewWithCapacity();
    stun::Writer writer(view);
    writer.WriteHeader(BindingType::kRequest);
    _transaction_tracker.SetTransactionId(view, 0);
    _transaction_hash = HeaderReader::GetTransactionIdHash(ToConst(view));
    stun_request.SetSize(writer.GetSize());

    _send_callback(_server, std::move(stun_request));
}

void StunClient::OnStunResponse(const BufferViewConst& view) {
    if(_reflexive) { return; }

    auto ok = Reader::ForEachAttribute(view, [this](AttributeType type, BufferViewConst attr) {
        if(type == AttributeType::kXorMappedAddress) {
            if(XorMappedAddressReader::GetFamily(attr) == IpFamily::kIpv4) {
                auto address = XorMappedAddressReader::GetAddressV4(attr);
                auto port = XorMappedAddressReader::GetPort(attr);
                _reflexive.emplace(Endpoint{asio_ip::address_v4(address), port});
            }
        }
        return true;
    });
    if(ok && _reflexive) {
        _candidate_callback(CandidateType::kServRefl, *_reflexive);
    } else {
        _reflexive.reset();
    }
}

Timepoint StunClient::WaitPeriodToNextRequest() const {
    if(!_reflexive || _transaction_tracker.HasTransaction(_transaction_hash)) {
        return kRtoDefault;
    }
    return kStunServerKeepAlivePeriod;
}

}
