#include "tau/sdp/line/attribute/Fmtp.h"
#include "tests/lib/Common.h"

namespace tau::sdp::attribute {

TEST(FmtpReaderTest, Validate) {
    ASSERT_TRUE(FmtpReader::Validate("102 level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42001f"));
    ASSERT_TRUE(FmtpReader::Validate("45 level-idx=5;profile=0;tier=0"));
    ASSERT_TRUE(FmtpReader::Validate("105 apt=104"));

    ASSERT_FALSE(FmtpReader::Validate("128 params"));
    ASSERT_FALSE(FmtpReader::Validate("100:params"));
    ASSERT_FALSE(FmtpReader::Validate("level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42001f"));
}

TEST(FmtpReaderTest, Basic) {
    std::string_view value = "96 packetization-mode=1;profile-level-id=42001f";
    ASSERT_EQ(96, FmtpReader::GetPt(value));
    ASSERT_EQ("packetization-mode=1;profile-level-id=42001f", FmtpReader::GetParameters(value));
}

TEST(FmtpWriterTest, Basic) {
    const auto value = FmtpWriter::Write(100, "hello=world");
    TAU_LOG_INFO("a=extmap:" << value);
    ASSERT_TRUE(FmtpReader::Validate(value));
    ASSERT_EQ(100, FmtpReader::GetPt(value));
    ASSERT_EQ("hello=world", FmtpReader::GetParameters(value));
}

}
