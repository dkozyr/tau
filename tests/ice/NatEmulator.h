#pragma once

#include "tau/memory/Buffer.h"
#include "tau/net/Endpoint.h"
#include <etl/string_stream.h>
#include <etl/unordered_map.h>
#include <etl/unordered_set.h>
#include <vector>
#include <functional>

namespace tau::ice {

using namespace tau::net;

// NAT emulator for tests only
class NatEmulator {
public:
    static inline const auto kPublicIpDefault = MakeIpAddressV4("1.1.1.1");

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

    etl::unordered_map<Endpoint, Endpoint, 6553> _local_to_public;
    etl::unordered_map<Endpoint, etl::unordered_set<Endpoint, 6553>, 6553> _public_to_remote;

    using LocalAndRemote = std::pair<Endpoint, Endpoint>;
    etl::unordered_map<Endpoint, LocalAndRemote, 6553> _public_to_local_remote; // symmetric case only

    std::vector<Context> _send_queue;

    Callback _on_send_callback;
    Callback _on_recv_callback;
};

etl::string_stream& operator<<(etl::string_stream& ss, const NatEmulator::Type& type);

}
