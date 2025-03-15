#include "tau/common/Base64.h"
#include "tests/Common.h"

TEST(Base64Test, Randomized) {
    for(size_t iteration = 0; iteration < 200; ++iteration) {
        std::vector<uint8_t> data;
        const auto size = g_random.Int<size_t>(1, 1024);
        for(size_t i = 0; i < (size - 1); ++i) {
            data.push_back(g_random.Int<uint8_t>());
        }
        data.push_back(g_random.Int<uint8_t>(1, 255)); //last must be non-zero

        const auto encoded = Base64Encode(data.data(), data.size());
        const auto decoded = Base64Decode(std::string_view(encoded));
        for(size_t i = 0; i < size; ++i) {
            ASSERT_EQ(data[i], static_cast<uint8_t>(decoded[i]));
        }
        ASSERT_EQ(size, decoded.size());
        // LOG_INFO << encoded;
    }
}

TEST(Base64Test, DecodeToHex) {
    //a=fmtp:96 packetization-mode=1;profile-level-id=640020;sprop-parameter-sets=Z2QAIKwsqAeAIl5ZuAgICgAAAwPoAACcQQg=,aO48sA==
    {
        const std::string encoded = "Z2QAIKwsqAeAIl5ZuAgICgAAAwPoAACcQQg=";
        const auto decoded = Base64Decode(encoded);
        std::stringstream ss;
        for(auto v : decoded) {
            ss << ToHexString(static_cast<uint8_t>(v)) << " ";
        }
        LOG_INFO << "[H264] sps: " << ss.str();
    }
    {
        const std::string encoded = "aO48sA==";
        const auto decoded = Base64Decode(encoded);
        std::stringstream ss;
        for(auto v : decoded) {
            ss << ToHexString(static_cast<uint8_t>(v)) << " ";
        }
        LOG_INFO << "[H264] pps: " << ss.str();
    }
}

TEST(Base64Test, Malformed) {
    const std::string encoded = "aO48sA%";
    ASSERT_ANY_THROW(Base64Decode(encoded));
}
