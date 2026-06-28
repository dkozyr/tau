#include "tau/crypto/Random.h"
#include "tests/lib/Common.h"

namespace tau::crypto {

TEST(RandomTest, Basic) {
    auto zeros = [](const auto& data) {
        return std::accumulate(data.begin(), data.end(), 0, [](size_t total, uint8_t value) {
            return total + (value == 0);
        });
    };

    const auto size = 123;
    etl::vector<uint8_t, 128> data(size, 0);
    ASSERT_EQ(size, zeros(data));

    ASSERT_TRUE(RandomBytes(data.data(), data.size()));
    ASSERT_GT(size, zeros(data));
}

TEST(RandomTest, RandomBase64) {
    etl::string<64> output;
    TAU_LOG_INFO("RandomBase64(4): " << RandomBase64(output, 4));
    TAU_LOG_INFO("RandomBase64(42): " << RandomBase64(output, 42));
    ASSERT_EQ(42, RandomBase64(output, 42).size());
    ASSERT_EQ(4, RandomBase64(output, 4).size());
    ASSERT_EQ(1, RandomBase64(output, 1).size());
    ASSERT_EQ(0, RandomBase64(output, 0).size());
}

TEST(RandomTest, RandomBase64_SmallCapacity) {
    etl::string<41> output;
    ASSERT_EQ(0, RandomBase64(output, 42).size());
    ASSERT_EQ(41, RandomBase64(output, 41).size());
}

}
