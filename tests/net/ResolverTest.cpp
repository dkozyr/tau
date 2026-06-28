#include "tau/net/Resolver.h"
#include "tests/lib/Common.h"

namespace tau::net {

TEST(ResolverTest, Basic) {
    {
        auto resolved = ResolveEndpointV4("stun.l.google.com", "19302");
        ASSERT_TRUE(resolved.has_value());
        TAU_LOG_INFO("Endpoint: " << ToString(*resolved));
        ASSERT_EQ(19302, resolved->port);
    }
    {
        auto resolved = ResolveEndpointV4("stun.xtau.com", "3478");
        ASSERT_TRUE(resolved.has_value());
        TAU_LOG_INFO("Endpoint: " << ToString(*resolved));
        ASSERT_EQ(3478, resolved->port);
    }
}

TEST(ResolverTest, Service) {
    {
        auto resolved = ResolveEndpointV4("google.com", "https");
        ASSERT_TRUE(resolved.has_value());
        TAU_LOG_INFO("Endpoint: " << ToString(*resolved));
        ASSERT_EQ(443, resolved->port);
    }
    {
        auto resolved = ResolveEndpointV4("xtau.com", {});
        ASSERT_TRUE(resolved.has_value());
        TAU_LOG_INFO("Endpoint: " << ToString(*resolved));
        ASSERT_EQ(0, resolved->port);
    }
}

}
