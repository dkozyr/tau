#include "tau/mdns/NameReader.h"
#include "tests/lib/Common.h"

namespace tau::mdns {

TEST(NameReaderTest, Basic) {
    std::vector<uint8_t> data = {
        4, 'T', 'e', 's', 't',
        0
    };
    const uint8_t* ptr = data.data();
    const uint8_t* end = ptr + data.size();
    ASSERT_EQ("Test", ParseName(ptr, end));
    ASSERT_EQ(ptr, end);
}

TEST(NameReaderTest, SeveralLabels) {
    std::vector<uint8_t> data = {
        5, 'H', 'e', 'l', 'l', 'o',
        5, 'w', 'o', 'r', 'l', 'd',
        2, '4', '2',
        5, 'l', 'o', 'c', 'a', 'l',
        0
    };
    const uint8_t* ptr = data.data();
    const uint8_t* end = ptr + data.size();
    ASSERT_EQ("Hello.world.42.local", ParseName(ptr, end));
    ASSERT_EQ(ptr, end);
}

TEST(NameReaderTest, CompressedFormat) {
    std::vector<uint8_t> data = {
        5, 'H', 'e', 'l', 'l', 'o',
        5, 'w', 'o', 'r', 'l', 'd',
        0xC0, 0x00,
        0xC0, 0x01,
        0xC0, 0x02
    };
    const uint8_t* ptr = data.data();
    const uint8_t* end = ptr + data.size();
    ASSERT_EQ("Hello.world.$.$.$", ParseName(ptr, end));
    ASSERT_EQ(ptr, end);
}

TEST(NameReaderTest, Empty) {
    std::vector<uint8_t> data = {
        0
    };
    const uint8_t* ptr = data.data();
    const uint8_t* end = ptr + data.size();
    ASSERT_TRUE(ParseName(ptr, end).empty());
    ASSERT_EQ(ptr, end);
}

TEST(NameReaderTest, Malformed) {
    std::vector<uint8_t> data = {
        5, 'H', 'e', 'l', 'l', 'o',
        6, 'w', 'o', 'r', 'l', 'd',
    };
    const uint8_t* ptr = data.data();
    const uint8_t* end = ptr + data.size();
    ASSERT_TRUE(ParseName(ptr, end).empty());
}

TEST(NameReaderTest, Malformed2) {
    std::vector<uint8_t> data = {
        1
    };
    const uint8_t* ptr = data.data();
    const uint8_t* end = ptr + data.size();
    ASSERT_TRUE(ParseName(ptr, end).empty());
}

TEST(NameReaderTest, MalformedCompressedFormat) {
    std::vector<uint8_t> data = {
        5, 'H', 'e', 'l', 'l', 'o',
        5, 'w', 'o', 'r', 'l', 'd',
        0xC0, 0x00,
        0xC0, 0x01,
        0xC0
    };
    const uint8_t* ptr = data.data();
    const uint8_t* end = ptr + data.size();
    ASSERT_TRUE(ParseName(ptr, end).empty());
}

}
