#include "tau/net/Resolver.h"
#include "tau/asio/ThreadPool.h"
#include "tests/lib/Common.h"

namespace tau::net {

TEST(ResolverTest, Basic) {
    ThreadPool io(std::thread::hardware_concurrency());

    auto resolved = Resolve(io.GetExecutor(), "stun.l.google.com", 19302);
    ASSERT_TRUE(resolved.has_value());
    for(auto& endpoint : *resolved) {
        TAU_LOG_INFO(endpoint.endpoint());
        ASSERT_EQ(19302, endpoint.endpoint().port());
    }
}

}
