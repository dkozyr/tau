#include "tau/sdp/line/Bandwidth.h"
#include "tests/lib/Common.h"

namespace tau::sdp {

TEST(BandwidthReaderTest, Validate) {
    ASSERT_TRUE(BandwidthReader::Validate("AS:1234"));
    ASSERT_TRUE(BandwidthReader::Validate("CT:234"));
    ASSERT_TRUE(BandwidthReader::Validate("X-TEST:1"));

    ASSERT_FALSE(BandwidthReader::Validate("AS:"));
    ASSERT_FALSE(BandwidthReader::Validate("AS:wrong"));
    ASSERT_FALSE(BandwidthReader::Validate("XX:1234"));
}

TEST(BandwidthReaderTest, Basic) {
    std::string_view value = "AS:1029";
    ASSERT_EQ("AS", BandwidthReader::GetType(value));
    ASSERT_EQ(1029, BandwidthReader::GetKbps(value));
}

TEST(BandwidthWriterTest, Basic) {
    const auto value = BandwidthWriter::Write("AS", 9876);
    LOG_INFO << "b=" << value;
    ASSERT_TRUE(BandwidthReader::Validate(value));
    ASSERT_EQ("AS", BandwidthReader::GetType(value));
    ASSERT_EQ(9876, BandwidthReader::GetKbps(value));
}

}
