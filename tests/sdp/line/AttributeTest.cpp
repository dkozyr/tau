#include "tau/sdp/line/Attribute.h"
#include "tau/sdp/line/attribute/RtcpFb.h"
#include "tests/lib/Common.h"

namespace tau::sdp {

TEST(AttributeReaderTest, Validate) {
    ASSERT_TRUE(AttributeReader::Validate("rtpmap:96 H264/90000"));
    ASSERT_TRUE(AttributeReader::Validate("rtcp:9 IN IP4 0.0.0.0"));
    ASSERT_TRUE(AttributeReader::Validate("ice-ufrag:bzRv+Hl9e/MnTuO7"));
    ASSERT_TRUE(AttributeReader::Validate("extmap:1 urn:ietf:params:rtp-hdrext:ssrc-audio-level"));
    ASSERT_TRUE(AttributeReader::Validate("sendrecv"));
    ASSERT_TRUE(AttributeReader::Validate("rtcp-mux"));
    ASSERT_TRUE(AttributeReader::Validate("rtpmap 96 malformed/but-valid-for-attribute-reader"));
    ASSERT_TRUE(AttributeReader::Validate("candidate:1521601408 1 udp 1686052607 1.1.1.1 63955 typ srflx raddr 192.168.0.1 rport 63955 generation 0"));

    ASSERT_FALSE(AttributeReader::Validate("extmap:1:malformed"));
    ASSERT_FALSE(AttributeReader::Validate("rtpmap:96:H264/90000"));
    ASSERT_FALSE(AttributeReader::Validate("rtcp-fb:128 wrong-pt"));
    ASSERT_FALSE(AttributeReader::Validate("fmtp:128 wrong-pt"));
    ASSERT_FALSE(AttributeReader::Validate("candidate:2896278100 2 udp 2122260222 1.2.3.4 59844 type host"));
}

TEST(AttributeReaderTest, Basic) {
    std::string_view value = "hello:world";
    ASSERT_EQ("hello", AttributeReader::GetType(value));
    ASSERT_EQ("world", AttributeReader::GetValue(value));
}

TEST(AttributeWriterTest, Basic) {
    const auto value = AttributeWriter::Write("hello", "world");
    LOG_INFO << "a=" << value;
    ASSERT_TRUE(AttributeReader::Validate(value));
    ASSERT_EQ("hello", AttributeReader::GetType(value));
    ASSERT_EQ("world", AttributeReader::GetValue(value));
}

TEST(AttributeWriterTest, KnownAttribute) {
    const auto value = AttributeWriter::Write("rtcp-fb", attribute::RtcpFbWriter::Write(100, "hello world"));
    LOG_INFO << "a=" << value;
    ASSERT_TRUE(AttributeReader::Validate(value));
    ASSERT_EQ("rtcp-fb", AttributeReader::GetType(value));
    const auto parsed_value = AttributeReader::GetValue(value);
    ASSERT_EQ(100, attribute::RtcpFbReader::GetPt(parsed_value));
    ASSERT_EQ("hello world", attribute::RtcpFbReader::GetValue(parsed_value));
}

}
