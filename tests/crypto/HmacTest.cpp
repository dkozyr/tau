#include "tau/crypto/Hmac.h"
#include "tests/lib/Common.h"

namespace tau::crypto {

TEST(HmacTest, Sha1) {
    std::vector<uint8_t> hash(kHmacSha1Length, 0);
    std::string data = "hello world";
    BufferViewConst view{.ptr = (const uint8_t*)data.data(), .size = data.size()};
    ASSERT_TRUE(HmacSha1(view, "password", hash.data()));
    TAU_LOG_INFO(ToHexDump(hash.data(), hash.size()));

    std::vector<uint8_t> target = {
        0x2F, 0xFA, 0xF1, 0xB5, 0xB3, 0xF8, 0x4A, 0x64,
        0x5C, 0xEB, 0xBC, 0x9B, 0x72, 0x49, 0x0C, 0x89,
        0xCB, 0x9F, 0x51, 0x9D
    };
    ASSERT_EQ(0, std::memcmp(target.data(), hash.data(), hash.size()));

    ASSERT_TRUE(HmacSha1(view, "top-secret", hash.data()));
    ASSERT_NE(0, std::memcmp(target.data(), hash.data(), hash.size()));
}

TEST(HmacTest, Sha256) {
    std::vector<uint8_t> hash(kHmacSha256Length, 0);
    std::string data = "hello world";
    BufferViewConst view{.ptr = (const uint8_t*)data.data(), .size = data.size()};
    ASSERT_TRUE(HmacSha256(view, "password", hash.data()));
    TAU_LOG_INFO(ToHexDump(hash.data(), hash.size()));

    std::vector<uint8_t> target = {
        0x8F, 0x5F, 0x35, 0x54, 0x41, 0xDC, 0x27, 0x22,
        0x90, 0x0F, 0x29, 0x20, 0x04, 0xF3, 0xD8, 0xA8,
        0x32, 0x45, 0xFF, 0x4D, 0x6E, 0x30, 0x78, 0xA5,
        0xB7, 0x7A, 0x4D, 0x7A, 0x92, 0x1E, 0xEA, 0xE9
    };
    ASSERT_EQ(0, std::memcmp(target.data(), hash.data(), hash.size()));

    ASSERT_TRUE(HmacSha1(view, "top-secret", hash.data()));
    ASSERT_NE(0, std::memcmp(target.data(), hash.data(), hash.size()));
}

}
