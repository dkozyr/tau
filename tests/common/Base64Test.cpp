#include "tau/common/Base64.h"
#include "tests/lib/Common.h"

namespace tau {

TEST(Base64Test, Randomized) {
    Random random;
    etl::string<256 * 8 / 6 + 4> encoded;
    etl::string<256> decoded;
    for(size_t iteration = 0; iteration < 200; ++iteration) {
        const auto size = random.Int<size_t>(1, 255);
        etl::vector<uint8_t, 255> data;
        for(size_t i = 0; i < (size - 1); ++i) {
            data.push_back(random.Int<uint8_t>());
        }
        data.push_back(random.Int<uint8_t>(1, 255)); //last must be non-zero

        Base64Encode(data.data(), data.size(), encoded);
        ASSERT_FALSE(encoded.empty());
        // TAU_LOG_INFO(encoded);

        Base64Decode(encoded, decoded);
        for(size_t i = 0; i < size; ++i) {
            ASSERT_EQ(data[i], static_cast<uint8_t>(decoded[i])) << "size: " << size;
        }
        ASSERT_EQ(size, decoded.size());
    }
}

TEST(Base64Test, DecodeToHex) {
    //a=fmtp:96 packetization-mode=1;profile-level-id=640020;sprop-parameter-sets=Z2QAIKwsqAeAIl5ZuAgICgAAAwPoAACcQQg=,aO48sA==
    {
        const etl::string_view encoded = "Z2QAIKwsqAeAIl5ZuAgICgAAAwPoAACcQQg=";
        etl::string<256> decoded;
        Base64Decode(encoded, decoded);
        ASSERT_FALSE(decoded.empty());

        etl::string<256> text;
        etl::string_stream ss(text);
        for(auto v : decoded) {
            const auto byte = static_cast<uint8_t>(v);
            ss << (byte < 0x10 ? "0" : "") << etl::hex << byte << " ";
        }
        TAU_LOG_INFO("[H264] sps: " << ss.str().c_str());
    }
    {
        const etl::string_view encoded = "aO48sA==";
        etl::string<256> decoded;
        Base64Decode(encoded, decoded);
        ASSERT_FALSE(decoded.empty());

        etl::string<256> text;
        etl::string_stream ss(text);
        for(auto v : decoded) {
            const auto byte = static_cast<uint8_t>(v);
            ss << (byte < 0x10 ? "0" : "") << etl::hex << byte << " ";
        }
        TAU_LOG_INFO("[H264] pps: " << ss.str());
    }
}

TEST(Base64Test, Malformed) {
    const etl::string_view encoded = "aO48sA%";
    etl::string<16> output;
    ASSERT_TRUE(Base64Decode(encoded, output).empty());
}

TEST(Base64Test, SmallCapacity) {
    const etl::string_view encoded = "aO48sA==";
    etl::string<1> output;
    ASSERT_TRUE(Base64Decode(encoded, output).empty());
}

}
