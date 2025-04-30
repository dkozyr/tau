#include "tau/net/Interface.h"
#include "tests/lib/Common.h"

namespace tau::net {

TEST(InterfaceTest, Basic) {
    auto interfaces = EnumerateInterfaces(true);
    for(auto& interface : interfaces) {
        TAU_LOG_INFO("Name: " << interface.name << ", address: " << interface.address);
    }
}

}
