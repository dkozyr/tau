#include "tests/ice/NatEmulator.h"
#include "tests/lib/Common.h"

namespace tau::ice {

class NatEmulatorTest : public ::testing::Test {
public:
    static inline auto kEndpoint1 = Endpoint{asio_ip::address_v4::from_string("1.2.3.4"), 33333};
    static inline auto kEndpoint2 = Endpoint{asio_ip::address_v4::from_string("2.3.4.5"), 44444};
    static inline auto kEndpoint3 = Endpoint{asio_ip::address_v4::from_string("3.4.5.6"), 55555};
    static inline auto kEndpoint4 = Endpoint{asio_ip::address_v4::from_string("4.5.6.7"), 65000};

    static inline auto kLocalEndpoint1 = Endpoint{asio_ip::address_v4::from_string("192.168.0.77"),  34567};
    static inline auto kLocalEndpoint2 = Endpoint{asio_ip::address_v4::from_string("192.168.0.111"), 45678};

    using Context = NatEmulator::Context;
    using Type = NatEmulator::Type;

protected:
    void Init(Type type) {
        _nat.emplace(_clock, NatEmulator::Options{.type = type});
        InitCallbacks();
    }

    void InitCallbacks() {
        _send.clear();
        _recv.clear();
        _nat->SetOnSendCallback([this](Buffer&& packet, Endpoint src, Endpoint dest) {
            _send.push_back(Context{.tp = _clock.Now(), .packet = std::move(packet), .src = src, .dest = dest});
        });
        _nat->SetOnRecvCallback([this](Buffer&& packet, Endpoint src, Endpoint dest) {
            _recv.push_back(Context{.tp = _clock.Now(), .packet = std::move(packet), .src = src, .dest = dest});
        });
    }

    void Process(Timepoint period = 1 * kSec) {
        const auto begin = _clock.Now();
        while(_clock.Now() < begin + period) {
            _clock.Add(5 * kMs);
            _nat->Process();
        }
    }

    void Wait(size_t send_count, size_t recv_count, Timepoint timeout = 1 * kSec) {
        const auto begin = _clock.Now();
        while((send_count != _send.size()) || (recv_count != _recv.size())) {
            ASSERT_TRUE(_clock.Now() < begin + timeout);
            _clock.Add(5 * kMs);
            _nat->Process();
        }
    }

    void Send(Endpoint dest, Endpoint src = kLocalEndpoint1) {
        _nat->Send(Buffer::Create(g_system_allocator, 100), src, dest);
    }

    void Recv(Endpoint src, Endpoint dest) {
        _nat->Recv(Buffer::Create(g_system_allocator, 100), src, dest);
    }

    static Endpoint BumpPort(Endpoint endpoint) {
        return Endpoint{endpoint.address(), (uint16_t)(endpoint.port() + 1)};
    }

protected:
    TestClock _clock;
    std::optional<NatEmulator> _nat;
    std::vector<Context> _send;
    std::vector<Context> _recv;
};

TEST_F(NatEmulatorTest, FullCone) {
    Init(Type::kFullCone);
    Send(kEndpoint1);
    Send(kEndpoint2);
    ASSERT_NO_FATAL_FAILURE(Wait(2, 0));
    ASSERT_EQ(_send[0].src, _send[1].src);

    Recv(kEndpoint1, _send[0].src);
    Recv(kEndpoint2, _send[0].src);
    Recv(kEndpoint3, _send[0].src);
    ASSERT_NO_FATAL_FAILURE(Wait(2, 3));

    Recv(kEndpoint1, BumpPort(_send[0].src));
    Process();
    ASSERT_NO_FATAL_FAILURE(Wait(2, 3));

    Send(kEndpoint2, kLocalEndpoint2);
    ASSERT_NO_FATAL_FAILURE(Wait(3, 3));
    ASSERT_EQ(BumpPort(_send[0].src), _send[2].src);
}

TEST_F(NatEmulatorTest, RestrictedCone) {
    Init(Type::kRestrictedCone);
    Send(kEndpoint1);
    Send(kEndpoint2);
    ASSERT_NO_FATAL_FAILURE(Wait(2, 0));
    ASSERT_EQ(_send[0].src, _send[1].src);

    Recv(BumpPort(kEndpoint1), _send[0].src);
    Recv(BumpPort(kEndpoint2), _send[0].src);
    ASSERT_NO_FATAL_FAILURE(Wait(2, 2));

    Recv(kEndpoint3, _send[0].src);
    Process();
    ASSERT_NO_FATAL_FAILURE(Wait(2, 2));
}

TEST_F(NatEmulatorTest, PortRestrictedCone) {
    Init(Type::kPortRestrictedCone);
    Send(kEndpoint1);
    Send(kEndpoint2);
    ASSERT_NO_FATAL_FAILURE(Wait(2, 0));
    ASSERT_EQ(_send[0].src, _send[1].src);

    Recv(kEndpoint1, _send[0].src);
    Recv(kEndpoint2, _send[0].src);
    ASSERT_NO_FATAL_FAILURE(Wait(2, 2));

    Recv(BumpPort(kEndpoint1), _send[0].src);
    Recv(BumpPort(kEndpoint2), _send[0].src);
    Recv(kEndpoint3, _send[0].src);
    Process();
    ASSERT_NO_FATAL_FAILURE(Wait(2, 2));
}

TEST_F(NatEmulatorTest, Symmetric) {
    Init(Type::kSymmetric);
    Send(kEndpoint1);
    Send(BumpPort(kEndpoint1));
    Send(kEndpoint2);
    ASSERT_NO_FATAL_FAILURE(Wait(3, 0));
    ASSERT_NE(_send[0].src, _send[1].src);
    ASSERT_NE(_send[0].src, _send[2].src);

    Recv(kEndpoint1,           _send[1].src);
    Recv(BumpPort(kEndpoint1), _send[0].src);
    Recv(kEndpoint2,           _send[0].src);
    Process();
    ASSERT_NO_FATAL_FAILURE(Wait(3, 0));

    Recv(kEndpoint1,           _send[0].src);
    Recv(BumpPort(kEndpoint1), _send[1].src);
    Recv(kEndpoint2,           _send[2].src);
    ASSERT_NO_FATAL_FAILURE(Wait(3, 3));
}

TEST_F(NatEmulatorTest, LocalNetworkOnly) {
    _nat.emplace(_clock, NatEmulator::Options{
        .type = Type::kLocalNetworkOnly,
        .public_ip = kLocalEndpoint1.address()
    });
    InitCallbacks();

    Send(kEndpoint1);
    Send(kEndpoint2);
    Process();
    ASSERT_NO_FATAL_FAILURE(Wait(0, 0));

    Recv(kEndpoint1, kLocalEndpoint1);
    Recv(kEndpoint2, kLocalEndpoint1);
    Recv(kEndpoint3, kLocalEndpoint1);
    ASSERT_NO_FATAL_FAILURE(Wait(0, 0));

    Send(kLocalEndpoint2);
    Process();
    ASSERT_NO_FATAL_FAILURE(Wait(1, 0));

    Recv(kLocalEndpoint2, kLocalEndpoint1);
    Recv(BumpPort(kLocalEndpoint2), kLocalEndpoint1);
    ASSERT_NO_FATAL_FAILURE(Wait(1, 2));
}

TEST_F(NatEmulatorTest, Delay) {
    constexpr auto kTestDelayMs = 42;
    _nat.emplace(_clock, NatEmulator::Options{
        .type = Type::kFullCone,
        .delay = kTestDelayMs * kMs
    });
    InitCallbacks();

    Send(kEndpoint1);
    for(size_t i = 0; i < kTestDelayMs - 1; ++i) {
        _clock.Add(1 * kMs);
        _nat->Process();
        ASSERT_EQ(0, _send.size());
    }
    _clock.Add(1 * kMs);
    _nat->Process();
    ASSERT_EQ(1, _send.size());
}

TEST_F(NatEmulatorTest, DropRate) {
    _nat.emplace(_clock, NatEmulator::Options{
        .type = Type::kFullCone,
        .drop_rate = 0.1
    });
    InitCallbacks();

    for(size_t i = 0; i < 100; ++i) {
        Send(kEndpoint1);
        _clock.Add(5 * kMs);
    }
    Process();
    const auto send_packets = _send.size();
    ASSERT_GT(100, send_packets);
    ASSERT_LT(75, send_packets);
    ASSERT_EQ(0, _recv.size());

    for(size_t i = 0; i < 100; ++i) {
        Recv(kEndpoint1, _send[0].src);
        _clock.Add(5 * kMs);
    }
    ASSERT_NO_FATAL_FAILURE(Wait(send_packets, 100));
}

}
