#include "tau/sdp/line/Originator.h"
#include "tests/lib/Common.h"

namespace tau::sdp {

TEST(OriginatorReaderTest, Validate) {
    ASSERT_TRUE(OriginatorReader::Validate("- 1742222222222222 1 IN IP4 192.168.0.1"));
    ASSERT_TRUE(OriginatorReader::Validate("- 1 1 IN IP6 0.0.0.0"));

    ASSERT_FALSE(OriginatorReader::Validate("- 1 1 IN IP4 0.0.0.0 suffix"));
    ASSERT_FALSE(OriginatorReader::Validate("- 1 1 XX IP4 0.0.0.0"));
    ASSERT_FALSE(OriginatorReader::Validate("- 1 1 IN IP 0.0.0.0"));
}

TEST(OriginatorReaderTest, Basic) {
    std::string_view value = "- 1742222222222222 1 IN IP4 192.168.0.1";
    ASSERT_EQ("IP4", OriginatorReader::GetAddressType(value));
    ASSERT_EQ("192.168.0.1", OriginatorReader::GetAddress(value));
}

TEST(OriginatorWriterTest, Basic) {
    const auto value = OriginatorWriter::Write("IP4", "123.45.67.89");
    TAU_LOG_INFO("o=" << value);
    ASSERT_TRUE(OriginatorReader::Validate(value));
    ASSERT_EQ("IP4", OriginatorReader::GetAddressType(value));
    ASSERT_EQ("123.45.67.89", OriginatorReader::GetAddress(value));
}

}
