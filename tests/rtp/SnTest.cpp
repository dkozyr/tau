#include "tau/rtp/Sn.h"
#include "tau/common/Log.h"
#include <gtest/gtest.h>

namespace rtp {

TEST(SnTest, Range) {
    SnRange range{.left = 65'531, .right = 200};
    for(uint16_t sn = 0; sn <= 200; ++sn) {
        ASSERT_TRUE(InRange(range, sn));
    }
    for(uint16_t sn = 201; sn <= 65'530; ++sn) {
        ASSERT_FALSE(InRange(range, sn));
    }
    for(uint16_t sn = 65'531; sn != 0; ++sn) {
        ASSERT_TRUE(InRange(range, sn));
    }
}

TEST(SnTest, Sn) {
    uint16_t sn = 100;
    ASSERT_EQ(110, SnForward(sn, 10));
    ASSERT_EQ(90, SnBackward(sn, 10));

    ASSERT_EQ(10'100, SnForward(sn, 10'000));
    ASSERT_EQ(55'636, SnBackward(sn, 10'000));

    ASSERT_EQ(90, SnForward(sn, 65'536 - 10));
    ASSERT_EQ(110, SnBackward(sn, 65'536 - 10));
}

TEST(SnTest, SnGreater) {
    ASSERT_TRUE(SnGreater(11, 10));
    ASSERT_TRUE(SnGreater(22222, 10));

    ASSERT_TRUE(SnGreater(kSnNegativeThreshold + 10, 10));
    ASSERT_FALSE(SnGreater(kSnNegativeThreshold + 10 + 1, 10));

    ASSERT_FALSE(SnGreater(65535, 10));
    ASSERT_FALSE(SnGreater(0, 10));
    ASSERT_FALSE(SnGreater(10, 10));

    for(uint16_t sn = 0; sn <= 10; sn++) {
        ASSERT_FALSE(SnGreater(sn, 10));    
    }
    for(uint16_t sn = 11; sn <= kSnNegativeThreshold + 10; sn++) {
        ASSERT_TRUE(SnGreater(sn, 10));    
    }
    for(uint16_t sn = kSnNegativeThreshold + 11; sn != 0; sn++) {
        ASSERT_FALSE(SnGreater(sn, 10));    
    }
}

TEST(SnTest, SnLesser) {
    ASSERT_TRUE(SnLesser(41, 42));
    ASSERT_TRUE(SnLesser(0, 42));
    ASSERT_TRUE(SnLesser(65535, 42));
    ASSERT_TRUE(SnLesser(44444, 42));

    ASSERT_TRUE(SnLesser(kSnNegativeThreshold + 42, 42));
    ASSERT_FALSE(SnLesser(kSnNegativeThreshold + 42 - 1, 42));

    ASSERT_FALSE(SnLesser(42, 42));
    ASSERT_FALSE(SnLesser(22222, 42));

    for(uint16_t sn = 0; sn < 42; sn++) {
        ASSERT_TRUE(SnLesser(sn, 42));    
    }
    for(uint16_t sn = 42; sn < kSnNegativeThreshold + 42; sn++) {
        ASSERT_FALSE(SnLesser(sn, 42));    
    }
    for(uint16_t sn = kSnNegativeThreshold + 42; sn != 0; sn++) {
        ASSERT_TRUE(SnLesser(sn, 42));    
    }
}

}
