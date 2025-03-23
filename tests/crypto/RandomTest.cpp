#include "tau/crypto/Random.h"
#include "tests/lib/Common.h"

namespace tau::crypto {

TEST(RandomTest, Basic) {
    auto zeros = [](const std::vector<uint8_t>& data) {
        return std::accumulate(data.begin(), data.end(), 0, [](size_t total, uint8_t value) {
            return total + (value == 0);
        });
    };

    const auto size = 123;
    std::vector<uint8_t> data(size, 0);
    ASSERT_EQ(size, zeros(data));

    ASSERT_TRUE(RandomBytes(data.data(), data.size()));
    ASSERT_GT(size, zeros(data));
}

}
