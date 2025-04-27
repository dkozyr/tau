#include "tau/mdns/Client.h"
#include "tau/common/Uuid.h"
#include "tests/lib/Common.h"

namespace tau::mdns {

class ClientTest : public ::testing::Test {
public:
    ClientTest()
        : _client1(Client::Dependencies{.udp_allocator = g_udp_allocator, .clock = _clock})
        , _client2(Client::Dependencies{.udp_allocator = g_udp_allocator, .clock = _clock})
    {
        InitCallbacks();
    }

    void InitCallbacks() {
        _client1.SetSendCallback([this](Buffer&& packet) { _client2.Recv(std::move(packet)); });
        _client2.SetSendCallback([this](Buffer&& packet) { _client1.Recv(std::move(packet)); });
    }

protected:
    TestClock _clock;
    Client _client1;
    Client _client2;
};

TEST_F(ClientTest, Basic) {
    const auto address1 = IpAddressV4(g_random.Int<uint32_t>());
    const auto name1 = _client1.CreateName(address1);
    TAU_LOG_INFO("Ip: " << address1 << ", name: " << name1);

    bool found = false;
    _client2.FindIpAddressByName(name1, [&](IpAddressV4 address) {
        found = true;
        EXPECT_EQ(address1, address);
    });
    ASSERT_TRUE(found);
}

TEST_F(ClientTest, NameNotFound) {
    const auto unknown_name = GenerateUuid() + ".local";
    TAU_LOG_INFO("Unknown name: " << unknown_name);

    bool found = false;
    _client1.FindIpAddressByName(unknown_name, [&](IpAddressV4) {
        found = true;
    });
    ASSERT_FALSE(found);
}

}
