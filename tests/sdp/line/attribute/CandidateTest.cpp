#include "tau/sdp/line/attribute/Candidate.h"
#include "tests/lib/Common.h"

namespace tau::sdp::attribute {

TEST(CandidateReaderTest, Validate) {
    ASSERT_TRUE(CandidateReader::Validate("2896278100 999 udp 2122260223 192.168.0.1 63955 typ host generation 0"));
    ASSERT_TRUE(CandidateReader::Validate("3793899172 1 tcp 1518280447 192.168.1.1 0 typ host tcptype active generation 0"));
    ASSERT_TRUE(CandidateReader::Validate("3793899172 2 tcp 1518280446 192.168.1.1 0 typ host tcptype active generation 0"));
    ASSERT_TRUE(CandidateReader::Validate("1521601408 1 udp 1686052607 1.1.1.1 63955 typ srflx raddr 192.168.0.1 rport 63955 generation 0"));
    ASSERT_TRUE(CandidateReader::Validate("1521601408 1 UDP 1686052607 1.1.1.1 63955 typ srflx"));

    ASSERT_FALSE(CandidateReader::Validate("2896278100 2 udp 2122260222 1.2.3.4 59844 type host"));
    ASSERT_FALSE(CandidateReader::Validate("2896278100 2 udp 2122260222 1.2.3.4 59844 typ "));
    ASSERT_FALSE(CandidateReader::Validate(" 2896278100 2 udp 2122260222 1.2.3.4 59844 typ host"));
    ASSERT_FALSE(CandidateReader::Validate("2896278100 X udp 2122260222 1.2.3.4 59844 typ host"));
    ASSERT_FALSE(CandidateReader::Validate("2896278100 1 udp xxxxxxxxxx 1.2.3.4 59844 typ host"));
    ASSERT_FALSE(CandidateReader::Validate("2896278100 1 udp 2122260222 1.2.3.4 123456 typ host"));
}

TEST(CandidateReaderTest, Basic) {
    std::string_view value = "2896278100 999 udp 2122260223 192.168.0.1 63955 typ host generation 0";
    ASSERT_EQ("2896278100", CandidateReader::GetFoundation(value));
    ASSERT_EQ(999, CandidateReader::GetComponentId(value));
    ASSERT_EQ("udp", CandidateReader::GetTransport(value));
    ASSERT_EQ(2122260223, CandidateReader::GetPriority(value));
    ASSERT_EQ("192.168.0.1", CandidateReader::GetAddress(value));
    ASSERT_EQ(63955, CandidateReader::GetPort(value));
    ASSERT_EQ("host", CandidateReader::GetType(value));
    ASSERT_EQ("generation 0", CandidateReader::GetExtParameters(value));
}

TEST(CandidateReaderTest, EmptyExtParameters) {
    std::string_view value = "2896278100 999 udp 2122260223 192.168.0.1 63955 typ host";
    ASSERT_EQ("", CandidateReader::GetExtParameters(value));
}

TEST(CandidateWriterTest, Basic) {
    const auto value = CandidateWriter::Write(1234567890, 22, "udp", 1234567890, "ip-port", 12345, "HOST", "extended params:hello-world");
    TAU_LOG_INFO("a=candidate:" << value);
    ASSERT_TRUE(CandidateReader::Validate(value));
    ASSERT_EQ("1234567890", CandidateReader::GetFoundation(value));
    ASSERT_EQ(22, CandidateReader::GetComponentId(value));
    ASSERT_EQ("udp", CandidateReader::GetTransport(value));
    ASSERT_EQ(1234567890, CandidateReader::GetPriority(value));
    ASSERT_EQ("ip-port", CandidateReader::GetAddress(value));
    ASSERT_EQ(12345, CandidateReader::GetPort(value));
    ASSERT_EQ("HOST", CandidateReader::GetType(value));
    ASSERT_EQ("extended params:hello-world", CandidateReader::GetExtParameters(value));
}

}
