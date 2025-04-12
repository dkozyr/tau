#include "tau/srtp/Common.h"
#include <gtest/gtest.h>

int main(int argc, char **argv) {
    tau::srtp::Init();

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
