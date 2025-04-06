#pragma once

#include "tau/memory/Buffer.h"
#include "tau/asio/Common.h"
#include <functional>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace tau::ice {

// NAT emulator for tests only
class NatEmulator {
public:
    static inline const auto kPublicIpDefault = IpAddress::from_string("1.1.1.1");

    enum class Type {
        kFullCone,
        kRestrictedCone,
        kPortRestrictedCone,
        kSymmetric,
        kLocalNetworkOnly
    };

    struct Options {
        Type type;
        IpAddress public_ip = kPublicIpDefault;
        Timepoint delay = 50 * kMs; // sending only
        double drop_rate = 0;       // [0, 1) sending only
    };

    struct Context {
        Timepoint tp;
        Buffer packet;
        Endpoint src;
        Endpoint dest;
    };

    using Callback = std::function<void(Buffer&&, Endpoint from, Endpoint to)>;

public:
    NatEmulator(Clock& clock, Options&& options);

    void SetOnSendCallback(Callback&& callback) { _on_send_callback = std::move(callback); }
    void SetOnRecvCallback(Callback&& callback) { _on_recv_callback = std::move(callback); }

    void Send(Buffer&& packet, Endpoint src, Endpoint dest);
    void Recv(Buffer&& packet, Endpoint src, Endpoint dest);

    void Process();

private:
    void OnSendFullConeNat(Buffer&& packet, Endpoint src, Endpoint dest);
    void OnSendRestrictedConeNat(Buffer&& packet, Endpoint src, Endpoint dest);
    void OnSendPortRestrictedConeNat(Buffer&& packet, Endpoint src, Endpoint dest);
    void OnSendSymmetricNat(Buffer&& packet, Endpoint src, Endpoint dest);
    void OnSendLocalNetworkOnly(Buffer&& packet, Endpoint src, Endpoint dest);

    void OnRecvFullConeNat(Buffer&& packet, Endpoint src, Endpoint dest);
    void OnRecvRestrictedConeNat(Buffer&& packet, Endpoint src, Endpoint dest);
    void OnRecvPortRestrictedConeNat(Buffer&& packet, Endpoint src, Endpoint dest);
    void OnRecvSymmetricNat(Buffer&& packet, Endpoint src, Endpoint dest);
    void OnRecvLocalNetworkOnly(Buffer&& packet, Endpoint src, Endpoint dest);

    void PushToQueue(Buffer&& packet, Endpoint src, Endpoint dest);

    static bool IsSameLocalNetwork(Endpoint a, Endpoint b);

private:
    Clock& _clock;
    const Options _options;

    uint16_t _latest_port = 33333;
    
    std::unordered_map<Endpoint, Endpoint> _local_to_public;
    std::unordered_map<Endpoint, std::unordered_set<Endpoint>> _public_to_remote;

    using LocalAndRemote = std::pair<Endpoint, Endpoint>;
    std::unordered_map<Endpoint, LocalAndRemote> _public_to_local_remote; // symmetric case only

    std::vector<Context> _send_queue;

    Callback _on_send_callback;
    Callback _on_recv_callback;
};

std::ostream& operator<<(std::ostream& s, const NatEmulator::Type& type);

}
