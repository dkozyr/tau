#include "tau/net/UdpSocket.h"
#include "tau/net/UdpSocketsPair.h"
#include "tau/memory/PoolAllocator.h"
#include "tau/asio/ThreadPool.h"
#include "tau/common/Event.h"
#include "tests/Common.h"

namespace tau::net {

class UdpSocketTest : public ::testing::Test {
public:
    UdpSocketTest()
        : _io(std::thread::hardware_concurrency())
    {}

    ~UdpSocketTest() {
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

TEST_F(UdpSocketTest, Basic) {
    constexpr auto kPacketSize1 = 1234;
    constexpr auto kPacketSize2 = 800;

    auto socket1 = UdpSocket::Create(
        UdpSocket::Options{
            .allocator = g_udp_allocator,
            .executor = _io.GetExecutor(),
            .local_address = IpAddress{.address = kLocalHost}
        });
    Event event1;
    socket1->SetRecvCallback([&](Buffer&& packet, asio_udp::endpoint remote_endpoint) {
        LOG_INFO << "[socket1] Received from: " << remote_endpoint << ", size: " << packet.GetSize();
        event1.Set();
        EXPECT_NO_FATAL_FAILURE(AssertPacket(packet, kPacketSize2));
    });

    auto socket2 = UdpSocket::Create(
        UdpSocket::Options{
            .allocator = g_udp_allocator,
            .executor = _io.GetExecutor(),
            .local_address = IpAddress{.address = kLocalHost}
        });
    Event event2;
    socket2->SetRecvCallback([&](Buffer&& packet, asio_udp::endpoint remote_endpoint) {
        LOG_INFO << "[socket2] Received from: " << remote_endpoint << ", size: " << packet.GetSize();
        event2.Set();
        EXPECT_NO_FATAL_FAILURE(AssertPacket(packet, kPacketSize1));
    });

    socket1->Send(CreatePacket(kPacketSize1), socket2->GetLocalEndpoint());
    socket2->Send(CreatePacket(kPacketSize2), socket1->GetLocalEndpoint());

    ASSERT_TRUE(event1.WaitFor(100ms));
    ASSERT_TRUE(event2.WaitFor(100ms));
}

TEST_F(UdpSocketTest, PortsPair) {
    auto [socket1, socket2] = CreateUdpSocketsPair(UdpSocket::Options{
        .allocator = g_udp_allocator,
        .executor = _io.GetExecutor(),
        .local_address = IpAddress{.address = kLocalHost}
    });
    auto endpoint1 = socket1->GetLocalEndpoint();
    auto endpoint2 = socket2->GetLocalEndpoint();
    LOG_INFO << "[socket1] endpoint: " << endpoint1;
    LOG_INFO << "[socket2] endpoint: " << endpoint2;
    ASSERT_EQ(endpoint1.port() + 1, endpoint2.port());
}

TEST_F(UdpSocketTest, DISABLED_MANUAL_Load) {
    constexpr auto kPacketSize1 = 1234;
    constexpr auto kPacketSize2 = 800;

    for(size_t i = 0; i < 10'000; ++i) {
        auto [socket1, socket2] = CreateUdpSocketsPair(UdpSocket::Options{
            .allocator = g_udp_allocator,
            .executor = _io.GetExecutor(),
            .local_address = IpAddress{.address = kLocalHost}
        });

        Event event1;
        socket1->SetRecvCallback([&](Buffer&&, asio_udp::endpoint) {
            event1.Set();
        });

        Event event2;
        socket2->SetRecvCallback([&](Buffer&&, asio_udp::endpoint) {
            event2.Set();
        });

        socket1->Send(CreatePacket(kPacketSize1), socket2->GetLocalEndpoint());
        socket2->Send(CreatePacket(kPacketSize2), socket1->GetLocalEndpoint());

        EXPECT_TRUE(event1.WaitFor(1000ms));
        EXPECT_TRUE(event2.WaitFor(1000ms));

        if(i % 1000 == 0) {
            LOG_INFO << "Iteration: #" << i;
        }
    }
}

}
