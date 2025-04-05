#include "tests/ice/NatEmulator.h"
#include "tests/lib/Common.h"

namespace tau::ice {

NatEmulator::NatEmulator(Clock& clock, Options&& options)
    : _clock(clock)
    , _options(std::move(options))
{}

void NatEmulator::Send(Buffer&& packet, Endpoint src, Endpoint dest) {
    switch(_options.type) {
        case Type::kFullCone:           return OnSendFullConeNat(std::move(packet), src, dest);
        case Type::kRestrictedCone:     return OnSendRestrictedConeNat(std::move(packet), src, dest);
        case Type::kPortRestrictedCone: return OnSendPortRestrictedConeNat(std::move(packet), src, dest);
        case Type::kSymmetric:          return OnSendSymmetricNat(std::move(packet), src, dest);
        case Type::kLocalNetworkOnly:   return OnSendLocalNetworkOnly(std::move(packet), src, dest);
        
    }
}

void NatEmulator::Recv(Buffer&& packet, Endpoint src, Endpoint dest) {
    switch(_options.type) {
        case Type::kFullCone:           return OnRecvFullConeNat(std::move(packet), src, dest);
        case Type::kRestrictedCone:     return OnRecvRestrictedConeNat(std::move(packet), src, dest);
        case Type::kPortRestrictedCone: return OnRecvPortRestrictedConeNat(std::move(packet), src, dest);
        case Type::kSymmetric:          return OnRecvSymmetricNat(std::move(packet), src, dest);
        case Type::kLocalNetworkOnly:   return OnRecvLocalNetworkOnly(std::move(packet), src, dest);
    }
}

void NatEmulator::Process() {
    const auto now = _clock.Now();
    while(!_send_queue.empty()) {
        auto& ctx = _send_queue[0];
        if(now < ctx.tp) {
            break;
        }
        if(g_random.Real(0.0, 1.0) >= _options.drop_rate) {
            _on_send_callback(std::move(ctx.packet), ctx.src, ctx.dest);
        } else {
            auto _ = std::move(ctx.packet);
        }
        _send_queue.erase(_send_queue.begin());
    }
}

void NatEmulator::OnSendFullConeNat(Buffer&& packet, Endpoint src, Endpoint dest) {
    if(auto it = _local_to_public.find(src); it != _local_to_public.end()) {
        auto& public_endpoint = it->second;
        PushToQueue(std::move(packet), public_endpoint, dest);
    } else {
        auto public_endpoint = Endpoint{_options.public_ip, _latest_port};
        _local_to_public[src] = public_endpoint;
        _public_to_remote[public_endpoint].insert(dest);
        ++_latest_port;
        PushToQueue(std::move(packet), public_endpoint, dest);
    }
}

void NatEmulator::OnSendRestrictedConeNat(Buffer&& packet, Endpoint src, Endpoint dest) {
    if(auto it = _local_to_public.find(src); it != _local_to_public.end()) {
        auto& public_endpoint = it->second;
        _public_to_remote[public_endpoint].insert(Endpoint{dest.address(), 0});
        PushToQueue(std::move(packet), public_endpoint, dest);
    } else {
        auto public_endpoint = Endpoint{_options.public_ip, _latest_port};
        _local_to_public[src] = public_endpoint;
        _public_to_remote[public_endpoint].insert(Endpoint{dest.address(), 0});
        ++_latest_port;
        PushToQueue(std::move(packet), public_endpoint, dest);
    }
}

void NatEmulator::OnSendPortRestrictedConeNat(Buffer&& packet, Endpoint src, Endpoint dest) {
    if(auto it = _local_to_public.find(src); it != _local_to_public.end()) {
        auto& public_endpoint = it->second;
        _public_to_remote[public_endpoint].insert(dest);
        PushToQueue(std::move(packet), public_endpoint, dest);
    } else {
        auto public_endpoint = Endpoint{_options.public_ip, _latest_port};
        _local_to_public[src] = public_endpoint;
        _public_to_remote[public_endpoint].insert(dest);
        ++_latest_port;
        PushToQueue(std::move(packet), public_endpoint, dest);
    }
}

void NatEmulator::OnSendSymmetricNat(Buffer&& packet, Endpoint src, Endpoint dest) {
    auto local_remote = std::make_pair(src, dest);
    for(auto& [public_endpoint, local_remote_pair] : _public_to_local_remote) {
        if(local_remote_pair == local_remote) {
            PushToQueue(std::move(packet), public_endpoint, dest);
            return;
        }
    }
    auto public_endpoint = Endpoint{_options.public_ip, _latest_port}; 
    _public_to_local_remote[public_endpoint] = local_remote;
    PushToQueue(std::move(packet), public_endpoint, dest);
    ++_latest_port;
}

void NatEmulator::OnSendLocalNetworkOnly(Buffer&& packet, Endpoint src, Endpoint dest) {
    if(IsSameLocalNetwork(src, dest) && (src.address() == _options.public_ip)) {
        PushToQueue(std::move(packet), src, dest);
    }
}

void NatEmulator::OnRecvFullConeNat(Buffer&& packet, Endpoint src, Endpoint dest) {
    for(auto& [local_src, pubic_endpoint] : _local_to_public) {
        if(dest == pubic_endpoint) {
            _on_recv_callback(std::move(packet), src, local_src);
            break;
        }
    }
}

void NatEmulator::OnRecvRestrictedConeNat(Buffer&& packet, Endpoint src, Endpoint dest) {
    for(auto& [local_src, pubic_endpoint] : _local_to_public) {
        if(dest == pubic_endpoint) {
            Endpoint src_ip{src.address(), 0};
            if(Contains(_public_to_remote.at(dest), src_ip)) {
                _on_recv_callback(std::move(packet), src, local_src);
            }
            break;
        }
    }
}

void NatEmulator::OnRecvPortRestrictedConeNat(Buffer&& packet, Endpoint src, Endpoint dest) {
    for(auto& [local_src, pubic_endpoint] : _local_to_public) {
        if(dest == pubic_endpoint) {
            if(Contains(_public_to_remote.at(dest), src)) {
                _on_recv_callback(std::move(packet), src, local_src);
            }
            break;
        }
    }
}

void NatEmulator::OnRecvSymmetricNat(Buffer&& packet, Endpoint src, Endpoint dest) {
    if(auto it = _public_to_local_remote.find(dest); it != _public_to_local_remote.end()) {
        auto& [local_src, expected_remote] = it->second;
        if(src == expected_remote) {
            _on_recv_callback(std::move(packet), src, local_src);
        }
    }
}

void NatEmulator::OnRecvLocalNetworkOnly(Buffer&& packet, Endpoint src, Endpoint dest) {
    if(IsSameLocalNetwork(src, dest) && (dest.address() == _options.public_ip)) {
        _on_recv_callback(std::move(packet), src, dest);
    }
}

void NatEmulator::PushToQueue(Buffer&& packet, Endpoint src, Endpoint dest) {
    _send_queue.push_back(Context{
        .tp = _clock.Now() + _options.delay,
        .packet = std::move(packet),
        .src = src,
        .dest = dest
    });
}

bool NatEmulator::IsSameLocalNetwork(Endpoint a, Endpoint b) {
    const auto a_uint = a.address().to_v4().to_uint() & 0xFFFFFF00;
    const auto b_uint = b.address().to_v4().to_uint() & 0xFFFFFF00;
    return (a_uint == b_uint);
}

}
