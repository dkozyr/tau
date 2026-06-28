#include "tau/crypto/Md5.h"
#include "tests/lib/Common.h"

namespace tau::crypto {

const etl::string_view kData = "hello world";

const uint8_t target[kMd5DigestLength] = {
    0x5e, 0xb6, 0x3b, 0xbb, 0xe0, 0x1e, 0xee, 0xd0,
    0x93, 0xcb, 0x22, 0xbb, 0x8f, 0x5a, 0xcd, 0xc3
};

TEST(Md5Test, Basic) {
    etl::vector<uint8_t, kMd5DigestLength> hash(kMd5DigestLength, 0);
    Md5Hasher md5;
    ASSERT_TRUE(md5.Update(kData));
    ASSERT_TRUE(md5.Finalize(hash.data()));

    etl::string<3 * kMd5DigestLength> dump;
    TAU_LOG_INFO(ToHexDump(hash.data(), hash.size(), dump));

    ASSERT_EQ(0, std::memcmp(target, hash.data(), hash.size()));
}

TEST(Md5Test, ChunksWithReset) {
    Md5Hasher md5;
    {
        ASSERT_TRUE(md5.Update(kData));

        etl::vector<uint8_t, kMd5DigestLength> hash(kMd5DigestLength, 0);
        ASSERT_TRUE(md5.Finalize(hash.data()));
        ASSERT_EQ(0, std::memcmp(target, hash.data(), hash.size()));
    }
    ASSERT_TRUE(md5.Reset());
    {
        for(size_t i = 0; i < kData.size(); ++i) {
            ASSERT_TRUE(md5.Update(kData.substr(i, 1)));
        }

        etl::vector<uint8_t, kMd5DigestLength> hash(kMd5DigestLength, 0);
        ASSERT_TRUE(md5.Finalize(hash.data()));
        ASSERT_EQ(0, std::memcmp(target, hash.data(), hash.size()));
    }
}

}
