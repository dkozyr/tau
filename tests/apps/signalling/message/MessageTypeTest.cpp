#include "apps/signalling/message/Type.h"
#include "tests/lib/Common.h"

namespace tau::signalling::message {

TEST(MessageTypeTest, FromString) {
    ASSERT_EQ(TypeFromString("init"), Type::kInit);
    ASSERT_EQ(TypeFromString("sdp"), Type::kSdp);
    ASSERT_EQ(TypeFromString("ice_candidates"), Type::kIceCandidates);
    ASSERT_EQ(TypeFromString("close"), Type::kClose);
    ASSERT_EQ(TypeFromString("wrong_value"), Type::kError);
}

TEST(MessageTypeTest, OutputOperator) {
    etl::string<64> output;
    etl::string_stream ss{output};
    ss << Type::kInit << ", " << Type::kSdp << ", " << Type::kIceCandidates << ", " << Type::kClose << ", " << Type::kError;
    ASSERT_EQ(output, "init, sdp, ice_candidates, close, error");
}

}
