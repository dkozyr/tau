#include "tau/sdp/line/attribute/RtcpFb.h"
#include "tests/lib/Common.h"

namespace tau::sdp::attribute {

TEST(RtcpFbReaderTest, Validate) {
    ASSERT_TRUE(RtcpFbReader::Validate("45 goog-remb"));
    ASSERT_TRUE(RtcpFbReader::Validate("95 transport-cc"));
    ASSERT_TRUE(RtcpFbReader::Validate("100 ccm fir"));
    ASSERT_TRUE(RtcpFbReader::Validate("120 nack"));
    ASSERT_TRUE(RtcpFbReader::Validate("127 nack pli"));

    ASSERT_FALSE(RtcpFbReader::Validate("128 nack"));
    ASSERT_FALSE(RtcpFbReader::Validate("100:nack"));
}

TEST(RtcpFbReaderTest, Basic) {
    std::string_view value = "96 nack pli";
    ASSERT_EQ(96, RtcpFbReader::GetPt(value));
    ASSERT_EQ("nack pli", RtcpFbReader::GetValue(value));
}

TEST(RtcpFbWriterTest, Basic) {
    const auto value = RtcpFbWriter::Write(100, "nack pli");
    LOG_INFO << "a=rtcp-fb:" << value;
    ASSERT_TRUE(RtcpFbReader::Validate(value));
    ASSERT_EQ(100, RtcpFbReader::GetPt(value));
    ASSERT_EQ("nack pli", RtcpFbReader::GetValue(value));
}

}
