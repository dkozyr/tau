#include "tau/crypto/Hmac.h"
#include "tests/lib/Common.h"

namespace tau::crypto {

struct HmacParams {
    HmacHasher::Type type;
};

const etl::array<HmacParams, 2> kHmacParamsVec = {
    HmacParams{.type = HmacHasher::Type::Sha1},
    HmacParams{.type = HmacHasher::Type::Sha256},
};

class HmacTest : public ::testing::TestWithParam<HmacParams> {
public:
    static constexpr size_t kHashMaxSize = kHmacSha256Length;

    static constexpr etl::string_view kData = "hello world";

    static constexpr uint8_t kSha1Target[kHmacSha1Length] = {
        0x2F, 0xFA, 0xF1, 0xB5, 0xB3, 0xF8, 0x4A, 0x64,
        0x5C, 0xEB, 0xBC, 0x9B, 0x72, 0x49, 0x0C, 0x89,
        0xCB, 0x9F, 0x51, 0x9D
    };

    static constexpr uint8_t kSha256Target[kHmacSha256Length] = {
        0x8F, 0x5F, 0x35, 0x54, 0x41, 0xDC, 0x27, 0x22,
        0x90, 0x0F, 0x29, 0x20, 0x04, 0xF3, 0xD8, 0xA8,
        0x32, 0x45, 0xFF, 0x4D, 0x6E, 0x30, 0x78, 0xA5,
        0xB7, 0x7A, 0x4D, 0x7A, 0x92, 0x1E, 0xEA, 0xE9
    };

protected:
    void Init(const etl::string_view& password) {
        _hasher.emplace(GetParam().type, password);
    }

    etl::vector<uint8_t, kHashMaxSize> InitHash() {
        etl::vector<uint8_t, kHashMaxSize> hash;
        switch(GetParam().type) {
            case HmacHasher::Type::Sha1:
                hash.resize(kHmacSha1Length);
                break;

            case HmacHasher::Type::Sha256:
                hash.resize(kHmacSha256Length);
                break;
        }
        return hash;
    }

    void AssertHash(const etl::ivector<uint8_t>& hash, bool valid = true) const {
        switch(GetParam().type) {
            case HmacHasher::Type::Sha1:
                ASSERT_EQ(kHmacSha1Length, hash.size());
                if(valid) {
                    ASSERT_EQ(0, std::memcmp(kSha1Target, hash.data(), hash.size()));
                } else {
                    ASSERT_NE(0, std::memcmp(kSha1Target, hash.data(), hash.size()));
                }
                break;

            case HmacHasher::Type::Sha256:
                ASSERT_EQ(kHmacSha256Length, hash.size());
                if(valid) {
                    ASSERT_EQ(0, std::memcmp(kSha256Target, hash.data(), hash.size()));
                } else {
                    ASSERT_NE(0, std::memcmp(kSha256Target, hash.data(), hash.size()));
                }
                break;
        }
    }

    void PrintHash(const etl::ivector<uint8_t>& hash) const {
        etl::string<3 * kHashMaxSize> dump;
        TAU_LOG_INFO(ToHexDump(hash.data(), hash.size(), dump));
    }

protected:
    std::optional<HmacHasher> _hasher;
};

INSTANTIATE_TEST_SUITE_P(Parameterized, HmacTest, ::testing::ValuesIn(kHmacParamsVec.begin(), kHmacParamsVec.end()));

TEST_P(HmacTest, Basic) {
    Init("password");

    auto view = ToViewConst(kData);
    ASSERT_TRUE(_hasher->Update(view));

    auto hash = InitHash();
    ASSERT_TRUE(_hasher->Finalize(hash.data()));

    PrintHash(hash);
    ASSERT_NO_FATAL_FAILURE(AssertHash(hash));
}

TEST_P(HmacTest, WrongPassword) {
    Init("wrong-password");

    auto view = ToViewConst(kData);
    ASSERT_TRUE(_hasher->Update(view));

    auto hash = InitHash();
    ASSERT_TRUE(_hasher->Finalize(hash.data()));

    PrintHash(hash);
    ASSERT_NO_FATAL_FAILURE(AssertHash(hash, false));
}

TEST_P(HmacTest, ChunksWithReset) {
    Init("password");

    auto view = ToViewConst(kData);
    {
        ASSERT_TRUE(_hasher->Update(view));

        auto hash = InitHash();
        ASSERT_TRUE(_hasher->Finalize(hash.data()));

        PrintHash(hash);
        ASSERT_NO_FATAL_FAILURE(AssertHash(hash));
    }
    ASSERT_TRUE(_hasher->Reset());
    {
        view.size = 1;
        for(size_t i = 0; i < kData.size(); ++i) {
            ASSERT_TRUE(_hasher->Update(view));
            view.ptr++;
        }

        auto hash = InitHash();
        ASSERT_TRUE(_hasher->Finalize(hash.data()));

        PrintHash(hash);
        ASSERT_NO_FATAL_FAILURE(AssertHash(hash));
    }
}

}
