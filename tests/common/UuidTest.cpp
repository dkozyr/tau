#include "tau/common/Uuid.h"
#include "tests/lib/Common.h"

namespace tau {

TEST(UuidTest, Basic) {
    auto uuid = GenerateUuid();
    TAU_LOG_INFO("UUID: " << uuid);
    ASSERT_TRUE(IsUuidTrivialCheck(uuid));

    uuid[2] = '-';
    ASSERT_FALSE(IsUuidTrivialCheck(uuid));
}

TEST(UuidTest, Validate) {
    ASSERT_TRUE(IsUuidTrivialCheck("3e2639ef-3a10-4dbd-8756-dd019f949086"));
    ASSERT_TRUE(IsUuidTrivialCheck("3E2639EF-3A10-4DBD-8756-DD019F949086"));

    ASSERT_FALSE(IsUuidTrivialCheck("3e2639ef 3a10-4dbd-8756-dd019f949086"));
    ASSERT_FALSE(IsUuidTrivialCheck("3e2639ef-3a10-4dbd-8756-dd01-f949086"));
    ASSERT_FALSE(IsUuidTrivialCheck("3e2639ef-3a10-8756-dd019f949086-4dbd"));

    ASSERT_FALSE(IsUuidTrivialCheck("3e2639ef 3a10-4dbd-8756-dd019f949086a"));
    ASSERT_FALSE(IsUuidTrivialCheck("3e2639ef 3a10-4dbd-8756-dd019f94908"));
    ASSERT_FALSE(IsUuidTrivialCheck("3e2639ef"));

    ASSERT_FALSE(IsUuidTrivialCheck("3e2639ef-3a10-4dbd-8756-dd019f949086 "));
}

}
