#include "tau/stun/Writer.h"
#include "tau/asio/Common.h"
#include "tests/lib/Common.h"

namespace tau::ice {

// TURN Server emulator for tests only
class TurnServerEmulator {
public:
    static inline const auto kPublicIpDefault = IpAddress::from_string("222.222.222.222");
    static inline const Endpoint kEndpointDefault = Endpoint{kPublicIpDefault, 3478};

    struct Options {
        IpAddress public_ip = kPublicIpDefault;
        std::string password = "password"; // same password for each client
    };

    using Callback = std::function<void(Buffer&&, Endpoint from, Endpoint to)>;

public:
    TurnServerEmulator(Clock& clock, Options&& options);

    void SetOnSendCallback(Callback&& callback) { _on_send_callback = std::move(callback); }

    void Recv(Buffer&& packet, Endpoint src, Endpoint dest);

    size_t GetDroppedPacketsCount() const;

private:
    void OnAllocateRequest(Buffer&& message, Endpoint src, uint32_t hash);
    void OnAllocateRequestInitial(Buffer&& message, Endpoint src, uint32_t hash);
    void OnAllocateRequest(Buffer&& message, Endpoint src, const std::string& user_name, const std::string& nonce);
    void OnRefreshRequest(Buffer&& message, Endpoint src);
    void OnCreatePermissionRequest(Buffer&& message, Endpoint src);
    void OnSendIndication(Buffer&& message, Endpoint src);

    void OnRecvData(Buffer&& packet, Endpoint src, Endpoint dest);

    void FinalizeStunMessage(Buffer& message, stun::Writer& writer, const std::string& user_name);

    void DropPacket(const std::string& message);

private:
    Clock& _clock;
    const Options _options;

    std::string _realm = "some_realm";

    uint16_t _latest_port = 33333;

    struct Allocation {
        std::string user_name;
        std::string nonce;
        uint16_t port;
        Timepoint expire_time;
        std::unordered_set<IpAddress> permissions = {};
    };

    std::unordered_map<uint32_t, std::string> _hash_to_nonce;
    std::unordered_map<Endpoint, Allocation> _client_to_allocation;
    size_t _dropped_packets_count = 0;

    Callback _on_send_callback;
};

}
