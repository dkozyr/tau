#include "tau/sdp/line/Connection.h"
#include "tests/lib/Common.h"

namespace tau::sdp {

TEST(ConnectionReaderTest, Validate) {
    ASSERT_TRUE(ConnectionReader::Validate("IN IP4 0.0.0.0"));
    ASSERT_TRUE(ConnectionReader::Validate("IN IP6 0:0:0:0"));
    ASSERT_TRUE(ConnectionReader::Validate("IN IP4 wrong_ip"));

    ASSERT_FALSE(ConnectionReader::Validate("IN IP 1.1.1.1"));
    ASSERT_FALSE(ConnectionReader::Validate("IN 1.1.1.1"));
    ASSERT_FALSE(ConnectionReader::Validate("XX IP4 1.1.1.1"));
}

TEST(ConnectionReaderTest, Basic) {
    std::string_view value = "IN IP4 11.22.33.44";
    ASSERT_EQ("IP4", ConnectionReader::GetAddressType(value));
    ASSERT_EQ("11.22.33.44", ConnectionReader::GetAddress(value));
}

TEST(ConnectionWriterTest, Basic) {
    const auto value = ConnectionWriter::Write("98.76.54.32.10.00");
    TAU_LOG_INFO("c=" << value);
    ASSERT_TRUE(ConnectionReader::Validate(value));
    ASSERT_EQ("IP4", ConnectionReader::GetAddressType(value));
    ASSERT_EQ("98.76.54.32.10.00", ConnectionReader::GetAddress(value));
}

}
