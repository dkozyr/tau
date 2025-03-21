#include "tau/sdp/line/attribute/Extmap.h"
#include "tests/lib/Common.h"

namespace tau::sdp::attribute {

TEST(ExtmapReaderTest, Validate) {
    ASSERT_TRUE(ExtmapReader::Validate("3 http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01"));
    ASSERT_TRUE(ExtmapReader::Validate("4 urn:ietf:params:rtp-hdrext:sdes:mid"));
    ASSERT_TRUE(ExtmapReader::Validate("5 http://www.webrtc.org/experiments/rtp-hdrext/playout-delay"));
    ASSERT_TRUE(ExtmapReader::Validate("5 uri:some:value"));

    ASSERT_FALSE(ExtmapReader::Validate("0 uri:some:value"));
    ASSERT_FALSE(ExtmapReader::Validate("15 uri:some:value"));
    ASSERT_FALSE(ExtmapReader::Validate("? uri"));
}

TEST(ExtmapReaderTest, Basic) {
    std::string_view value = "1/recvonly urn:ietf:params:rtp-hdrext:sdes:mid";
    ASSERT_EQ(1, ExtmapReader::GetId(value));
    ASSERT_EQ(Direction::kRecv, ExtmapReader::GetDirection(value));
    ASSERT_EQ("urn:ietf:params:rtp-hdrext:sdes:mid", ExtmapReader::GetUri(value));
}

TEST(ExtmapReaderTest, Direction) {
    ASSERT_EQ(Direction::kInactive, ExtmapReader::GetDirection("1/inactive uri"));
    ASSERT_EQ(Direction::kRecv,     ExtmapReader::GetDirection("1/recvonly uri"));
    ASSERT_EQ(Direction::kSend,     ExtmapReader::GetDirection("1/sendonly uri"));
    ASSERT_EQ(Direction::kSendRecv, ExtmapReader::GetDirection("1 uri"));
    ASSERT_EQ(Direction::kSendRecv, ExtmapReader::GetDirection("1/malformed uri"));
}

TEST(ExtmapWriterTest, Basic) {
    const auto value = ExtmapWriter::Write(8, "uri:some:path");
    LOG_INFO << "a=extmap:" << value;
    ASSERT_TRUE(ExtmapReader::Validate(value));
    ASSERT_EQ(8, ExtmapReader::GetId(value));
    ASSERT_EQ(Direction::kSendRecv, ExtmapReader::GetDirection(value));
    ASSERT_EQ("uri:some:path", ExtmapReader::GetUri(value));
}

TEST(ExtmapWriterTest, BasicWithDirection) {
    const auto value = ExtmapWriter::Write(14, "uri:some:path", Direction::kSend);
    LOG_INFO << "a=extmap:" << value;
    ASSERT_TRUE(ExtmapReader::Validate(value));
    ASSERT_EQ(14, ExtmapReader::GetId(value));
    ASSERT_EQ(Direction::kSend, ExtmapReader::GetDirection(value));
    ASSERT_EQ("uri:some:path", ExtmapReader::GetUri(value));
}

}
