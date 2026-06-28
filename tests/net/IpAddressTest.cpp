#include "tau/net/IpAddress.h"
#include "tests/lib/Common.h"

namespace tau::net {

TEST(IpAddressTest, Basic) {
    auto ip1 = IpAddress(127, 07, 255, 99);
    auto ip2 = MakeIpAddressV4("127.07.255.99");
    auto ip3 = IpAddress{0x7F07FF63};
    auto ip4 = IpAddress{0x63FF077F, true};
    ASSERT_EQ(ip1, ip2);
    ASSERT_EQ(ip1, ip3);
    ASSERT_EQ(ip1, ip4);

    ASSERT_NE(ip1, MakeIpAddressV4("127.0.255.99"));
}

}
