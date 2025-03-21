#include "tau/sdp/line/attribute/Rtpmap.h"
#include "tests/lib/Common.h"

namespace tau::sdp::attribute {

TEST(RtpmapReaderTest, Validate) {
    ASSERT_TRUE(RtpmapReader::Validate("102 H264/90000"));
    ASSERT_TRUE(RtpmapReader::Validate("96 VP8/90000"));
    ASSERT_TRUE(RtpmapReader::Validate("9 G722/8000"));
    ASSERT_TRUE(RtpmapReader::Validate("0 PCMU/8000"));
    ASSERT_TRUE(RtpmapReader::Validate("127 opus/48000/2"));
    ASSERT_TRUE(RtpmapReader::Validate("126 telephone-event/48000"));

    ASSERT_FALSE(RtpmapReader::Validate("128 H264/90000"));
    ASSERT_FALSE(RtpmapReader::Validate("100 H264 90000"));
    ASSERT_FALSE(RtpmapReader::Validate("100 H264:90000"));
}

TEST(RtpmapReaderTest, Basic) {
    std::string_view value = "127 telephone-event/12345";
    ASSERT_EQ(127, RtpmapReader::GetPt(value));
    ASSERT_EQ("telephone-event", RtpmapReader::GetEncodingName(value));
    ASSERT_EQ(12345, RtpmapReader::GetClockRate(value));
    ASSERT_EQ(std::string_view{}, RtpmapReader::GetParams(value));
}

TEST(RtpmapWriterTest, Basic) {
    const auto value = RtpmapWriter::Write(100, "H264", 90000);
    LOG_INFO << "a=rtpmap:" << value;
    ASSERT_TRUE(RtpmapReader::Validate(value));
    ASSERT_EQ(100, RtpmapReader::GetPt(value));
    ASSERT_EQ("H264", RtpmapReader::GetEncodingName(value));
    ASSERT_EQ(90000, RtpmapReader::GetClockRate(value));
    ASSERT_EQ(std::string_view{}, RtpmapReader::GetParams(value));
}

TEST(RtpmapWriterTest, Opus) {
    const auto value = RtpmapWriter::Write(96, "opus", 48000, "2");
    LOG_INFO << "a=rtpmap:" << value;
    ASSERT_TRUE(RtpmapReader::Validate(value));
    ASSERT_EQ(96, RtpmapReader::GetPt(value));
    ASSERT_EQ("opus", RtpmapReader::GetEncodingName(value));
    ASSERT_EQ(48000, RtpmapReader::GetClockRate(value));
    ASSERT_EQ("2", RtpmapReader::GetParams(value));
}

}
