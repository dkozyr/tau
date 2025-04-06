#include "tau/ice/TurnClient.h"
#include "tests/ice/TurnServerEmulator.h"
#include "tests/ice/NatEmulator.h"
#include "tau/net/Interface.h"
#include "tau/net/UdpSocket.h"
#include "tau/asio/ThreadPool.h"
#include "tau/common/Event.h"

namespace tau::ice {

class TurnClientTest : public ::testing::Test {
public:
    static inline Endpoint kClientEndpoint{asio_ip::address_v4::from_string("192.168.0.77"), 44444};
    static inline Endpoint kServerEndpoint = TurnServerEmulator::kEndpointDefault;

public:
    TurnClientTest()
        : _nat(_clock, NatEmulator::Options{.type = NatEmulator::Type::kSymmetric})
        , _turn_server(_clock, TurnServerEmulator::Options{})
    {}

    ~TurnClientTest() {
        EXPECT_EQ(0, _turn_server.GetDroppedPacketsCount());
    }

    void Init() {
        _client.emplace(
            TurnClient::Dependencies{
                .clock = _clock,
                .udp_allocator = g_udp_allocator
            },
            TurnClient::Options{_options}
        );
        InitCallbacks();
    }

    void InitCallbacks() {
        _client->SetCandidateCallback([this](Endpoint remote) {
            _local_candidates.push_back(remote);
        });
        _client->SetSendCallback([this](Endpoint remote, Buffer&& message) {
            _nat.Send(std::move(message), kClientEndpoint, remote);
            _send_packets_count++;
        });
        _client->SetRecvCallback([this](Endpoint remote, Buffer&& message) {
            _from_remote_peer_packets.push_back(std::make_pair(remote, std::move(message)));
        });
        _nat.SetOnSendCallback([this](Buffer&& message, Endpoint src, Endpoint dest) {
            if(dest == kServerEndpoint) {
                _turn_server.Recv(std::move(message), src, dest);
            }
        });
        _nat.SetOnRecvCallback([this](Buffer&& message, Endpoint src, Endpoint) {
            if(_client->IsServerEndpoint(src)) {
                _client->Recv(std::move(message));
            }
        });
        _turn_server.SetOnSendCallback([this](Buffer&& message, Endpoint src, Endpoint dest) {
            if(dest.address() == NatEmulator::kPublicIpDefault) {
                _nat.Recv(std::move(message), src, dest);
            } else {
                _to_remote_peer_packets.push_back(std::make_pair(dest, std::move(message)));
            }
        });
    }

    void ProcessNat(size_t period_ms = 50) {
        _clock.Add(period_ms * kMs);
        _nat.Process();
    }

    static Buffer CreatePacket(size_t size = 100) {
        auto packet = Buffer::Create(g_udp_allocator, size);
        for(size_t i = 0; i < size; ++i) {
            packet.GetView().ptr[i] = i;
        }
        packet.SetSize(size);
        return packet;
    }

protected:
    TestClock _clock;

    TurnClient::Options _options = {
        .server = TurnServerEmulator::kEndpointDefault,
        .credentials = {
            .ufrag = "username",
            .password = "password"
        }
    };
    std::optional<TurnClient> _client;
    NatEmulator _nat;
    TurnServerEmulator _turn_server;

    std::vector<Endpoint> _local_candidates;
    size_t _send_packets_count = 0;
    std::vector<std::pair<Endpoint, Buffer>> _from_remote_peer_packets;
    std::vector<std::pair<Endpoint, Buffer>> _to_remote_peer_packets;
};

TEST_F(TurnClientTest, IsServerEndpoint) {
    Init();
    ASSERT_TRUE(_client->IsServerEndpoint(kServerEndpoint));
    ASSERT_FALSE(_client->IsServerEndpoint(kClientEndpoint));
}

TEST_F(TurnClientTest, BasicAllocation) {
    Init();

    _client->Process();
    ProcessNat();
    ASSERT_EQ(0, _send_packets_count);

    _clock.Add(_options.start_delay);
    _client->Process();
    ProcessNat();
    ASSERT_EQ(1, _send_packets_count);
    ASSERT_EQ(0, _local_candidates.size());

    _clock.Add(50 * kMs);
    _client->Process();
    ProcessNat();
    ASSERT_EQ(2, _send_packets_count);
    ASSERT_EQ(1, _local_candidates.size());
    const auto& relayed = _local_candidates.back();
    ASSERT_EQ(TurnServerEmulator::kPublicIpDefault, relayed.address());

    Endpoint remote_peer{asio_ip::address_v4::from_string("55.66.77.88"), 54321};
    _client->CreatePermission(remote_peer.address());
    ASSERT_EQ(3, _send_packets_count);
    ASSERT_FALSE(_client->HasPermission(remote_peer.address()));
    ProcessNat();
    ASSERT_TRUE(_client->HasPermission(remote_peer.address()));

    auto outgoing_packet = CreatePacket();
    _client->Send(outgoing_packet.MakeCopy(), remote_peer);
    ProcessNat();
    ASSERT_EQ(4, _send_packets_count);
    ASSERT_EQ(1, _to_remote_peer_packets.size());
    auto& [dest, packet] = _to_remote_peer_packets.back();
    ASSERT_EQ(remote_peer, dest);
    ASSERT_EQ(outgoing_packet.GetSize(), packet.GetSize());
    ASSERT_EQ(0, std::memcmp(outgoing_packet.GetView().ptr, packet.GetView().ptr, packet.GetSize()));

    auto incoming_packet = CreatePacket(1234);
    _turn_server.Recv(incoming_packet.MakeCopy(), remote_peer, relayed);
    ASSERT_EQ(1, _from_remote_peer_packets.size());
    auto& [from_remote, recv_packet] = _from_remote_peer_packets.back();
    ASSERT_EQ(remote_peer, from_remote);
    ASSERT_EQ(incoming_packet.GetSize(), recv_packet.GetSize());
    ASSERT_EQ(0, std::memcmp(incoming_packet.GetView().ptr, recv_packet.GetView().ptr, recv_packet.GetSize()));

    _client->Stop();
    ProcessNat();
    ASSERT_EQ(5, _send_packets_count);
}

// turnserver --allow-loopback-peers --cli-password=nonempty --lt-cred-mech --user=username:password --realm testrealm --log-file stdout -v
TEST_F(TurnClientTest, DISABLED_MANUAL_Coturn) {
    ThreadPool io(1);
    SteadyClock clock;

    net::UdpSocketPtr udp_socket;
    auto interfaces = net::EnumerateInterfaces(true);
    for(auto& interface : interfaces) {
        LOG_INFO << "Name: " << interface.name << ", address: " << interface.address;
        if(IsPrefix(interface.name, "wlo")) {
            udp_socket = net::UdpSocket::Create(net::UdpSocket::Options{
                .allocator = g_udp_allocator,
                .executor = io.GetExecutor(),
                .local_address = {.address = interface.address.to_string()}
            });
            LOG_INFO << "Local endpoint: " << udp_socket->GetLocalEndpoint();
            break;
        }
    }

    TurnClient client(
        TurnClient::Dependencies{
            .clock = clock, .udp_allocator = g_udp_allocator
        },
        TurnClient::Options{
            .server = Endpoint{asio_ip::address_v4::from_string("127.0.0.1"), 3478},
            .credentials = {
                .ufrag = "username",
                .password = "password"
            },
            .log_ctx = "[test] "
        });

    client.SetCandidateCallback([&](Endpoint relayed) {
        LOG_INFO << "candidate relayed: " << relayed;
    });
    client.SetSendCallback([&](Endpoint remote, Buffer&& message) {
        udp_socket->Send(std::move(message), remote);
    });
    udp_socket->SetRecvCallback([&](Buffer&& packet, Endpoint remote_endpoint) {
        LOG_INFO << "[Recv] remote: " << remote_endpoint << ", packet: " << packet.GetSize();
        if(client.IsServerEndpoint(remote_endpoint)) {
            client.Recv(std::move(packet));
        } else {
            LOG_WARNING << "Wrong server endpoint";
        }
    });

    for(size_t i = 1; i < 50; ++i) { 
        client.Process();
        std::this_thread::sleep_for(100ms);
    }


    Endpoint remote_peer{asio_ip::address_v4::from_string("192.168.0.154"), 54321};
    client.CreatePermission(remote_peer.address());
    while(!client.HasPermission(remote_peer.address())) {
        std::this_thread::sleep_for(100ms);
    }

    client.Send(CreatePacket(), remote_peer);
    std::this_thread::sleep_for(100ms);

    client.Stop();
    Event().WaitFor(150ms);
}

}
