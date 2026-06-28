#include "tau/net/UdpSocketWithExecutor.h"
#include "tau/net/UdpSocketsPair.h"
#include "tau/asio/ThreadPool.h"
#include "tau/asio/ToString.h"
#include "tau/common/Event.h"
#include "tests/lib/Common.h"

namespace tau::net {

class UdpSocketWithExecutorTest : public ::testing::Test {
public:
    static inline const IpAddress kLocalHost{MakeIpAddressV4("127.0.0.1")};

public:
    UdpSocketWithExecutorTest()
        : _io(std::thread::hardware_concurrency())
    {}

    ~UdpSocketWithExecutorTest() {
        _io.Join();
    }

protected:
    Buffer CreatePacket(size_t size) {
        auto packet = Buffer::Create(g_udp_allocator);
        packet.SetSize(size);
        auto view = packet.GetView();
        for(size_t i = 0; i < size; ++i) {
            view.ptr[i] = static_cast<uint8_t>(i);
        }
        return packet;
    }

    static void AssertPacket(const Buffer& packet, size_t target_size) {
        const auto view = packet.GetView();
        EXPECT_EQ(target_size, view.size);
        for(size_t i = 0; i < target_size; ++i) {
            EXPECT_EQ(static_cast<uint8_t>(i), view.ptr[i]);
        }
    }

protected:
    ThreadPool _io;
};

TEST_F(UdpSocketWithExecutorTest, Basic) {
    constexpr auto kPacketSize1 = 1234;
    constexpr auto kPacketSize2 = 800;

    auto socket1 = UdpSocketWithExecutor::Create(
        UdpSocketWithExecutor::Options{
            .allocator = g_udp_allocator,
            .executor = _io.GetExecutor(),
            .local_address = kLocalHost
        });
    Event event1;
    socket1->SetRecvCallback([&](Buffer&& packet, Endpoint remote_endpoint) {
        TAU_LOG_INFO("[socket1] Received from: " << remote_endpoint << ", size: " << packet.GetSize());
        event1.Set();
        EXPECT_NO_FATAL_FAILURE(AssertPacket(packet, kPacketSize2));
    });

    auto socket2 = UdpSocketWithExecutor::Create(
        UdpSocketWithExecutor::Options{
            .allocator = g_udp_allocator,
            .executor = _io.GetExecutor(),
            .local_address = kLocalHost
        });
    Event event2;
    socket2->SetRecvCallback([&](Buffer&& packet, Endpoint remote_endpoint) {
        TAU_LOG_INFO("[socket2] Received from: " << remote_endpoint << ", size: " << packet.GetSize());
        event2.Set();
        EXPECT_NO_FATAL_FAILURE(AssertPacket(packet, kPacketSize1));
    });

    socket1->Send(CreatePacket(kPacketSize1), socket2->GetLocalEndpoint().value());
    socket2->Send(CreatePacket(kPacketSize2), socket1->GetLocalEndpoint().value());

    ASSERT_TRUE(event1.WaitFor(100ms));
    ASSERT_TRUE(event2.WaitFor(100ms));
}

TEST_F(UdpSocketWithExecutorTest, PortsPair) {
    auto [socket1, socket2] = CreateUdpSocketsPair<UdpSocketWithExecutor>(
        UdpSocketWithExecutor::Options{
            .allocator = g_udp_allocator,
            .executor = _io.GetExecutor(),
            .local_address = kLocalHost
        });
    auto endpoint1 = socket1->GetLocalEndpoint().value();
    auto endpoint2 = socket2->GetLocalEndpoint().value();
    TAU_LOG_INFO("[socket1] endpoint: " << endpoint1);
    TAU_LOG_INFO("[socket2] endpoint: " << endpoint2);
    ASSERT_EQ(endpoint1.port + 1, endpoint2.port);
}

TEST_F(UdpSocketWithExecutorTest, Load) {
    constexpr auto kPacketSize1 = 1234;
    constexpr auto kPacketSize2 = 800;

    auto [socket1, socket2] = CreateUdpSocketsPair<UdpSocketWithExecutor>(
        UdpSocketWithExecutor::Options{
            .allocator = g_udp_allocator,
            .executor = _io.GetExecutor(),
            .local_address = kLocalHost
        });

    for(size_t i = 0; i < 10'000; ++i) {
        Event event1;
        socket1->SetRecvCallback([&](Buffer&&, Endpoint) {
            event1.Set();
        });

        Event event2;
        socket2->SetRecvCallback([&](Buffer&&, Endpoint) {
            event2.Set();
        });

        socket1->Send(CreatePacket(kPacketSize1), socket2->GetLocalEndpoint().value());
        socket2->Send(CreatePacket(kPacketSize2), socket1->GetLocalEndpoint().value());

        EXPECT_TRUE(event1.WaitFor(1000ms));
        EXPECT_TRUE(event2.WaitFor(1000ms));

        if(i % 1000 == 0) {
            TAU_LOG_INFO("Iteration: #" << i);
        }
    }
}

TEST_F(UdpSocketWithExecutorTest, DISABLED_MANUAL_Multicast) {
    auto mdns_socket = UdpSocketWithExecutor::Create(
        UdpSocketWithExecutor::Options{
            .allocator = g_udp_allocator,
            .executor = _io.GetExecutor(),
            .local_address = {},
            .local_port = 5353,                            // mDns port
            .multicast_address = IpAddress{224, 0, 0, 251} // mDns IPv4
        });

    mdns_socket->SetRecvCallback([&](Buffer&& packet, Endpoint remote_endpoint) {
        TAU_LOG_INFO("Multicast remote endpoint: " << remote_endpoint << ", packet size: " << packet.GetSize());
    });

    Event().WaitFor(600s);
}

}
