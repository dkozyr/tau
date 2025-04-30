#include "tau/crypto/Md5.h"
#include "tests/lib/Common.h"

namespace tau::crypto {

TEST(Md5Test, Basic) {
    std::vector<uint8_t> hash(kMd5DigestLength, 0);
    std::string data = "hello world";
    ASSERT_TRUE(Md5(data, hash.data()));
    TAU_LOG_INFO(ToHexDump(hash.data(), hash.size()));

    std::vector<uint8_t> target = {
        0x5e, 0xb6, 0x3b, 0xbb, 0xe0, 0x1e, 0xee, 0xd0,
        0x93, 0xcb, 0x22, 0xbb, 0x8f, 0x5a, 0xcd, 0xc3
    };
    ASSERT_EQ(0, std::memcmp(target.data(), hash.data(), hash.size()));
}

}
