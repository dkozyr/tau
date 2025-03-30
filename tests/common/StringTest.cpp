#include "tau/common/String.h"
#include "tests/lib/Common.h"

namespace tau {

TEST(StringTest, StringToUnsigned) {
    ASSERT_EQ(1234567890, StringToUnsigned(std::string_view{"01234567890"}));
    ASSERT_EQ(0, StringToUnsigned(std::string_view{"0"}));
    ASSERT_FALSE(StringToUnsigned(std::string_view{"012345-67890"}).has_value());
    ASSERT_FALSE(StringToUnsigned(std::string_view{"-12345"}).has_value());
    ASSERT_FALSE(StringToUnsigned(std::string_view{}).has_value());
    ASSERT_FALSE(StringToUnsigned(std::string_view{"a"}).has_value());
}

TEST(StringTest, StringToUnsigned_DifferentType) {
    ASSERT_EQ(18446744073709551615ull, StringToUnsigned<uint64_t>(std::string_view{"18446744073709551615"}).value());
    ASSERT_EQ(4294967296ull, StringToUnsigned<uint64_t>(std::string_view{"4294967296"}).value());
    ASSERT_EQ(4294967295u, StringToUnsigned<uint32_t>(std::string_view{"4294967295"}).value());
    ASSERT_FALSE(StringToUnsigned<uint32_t>(std::string_view{"4294967296"}).has_value());

    ASSERT_EQ(65536, StringToUnsigned<uint64_t>(std::string_view{"65536"}).value());
    ASSERT_EQ(65536, StringToUnsigned<uint32_t>(std::string_view{"65536"}).value());
    ASSERT_FALSE(StringToUnsigned<uint16_t>(std::string_view{"65536"}).has_value());
    ASSERT_FALSE(StringToUnsigned<uint8_t>(std::string_view{"65536"}).has_value());

    ASSERT_EQ(256, StringToUnsigned<uint16_t>(std::string_view{"256"}).value());
    ASSERT_EQ(255, StringToUnsigned<uint8_t>(std::string_view{"255"}).value());
    ASSERT_FALSE(StringToUnsigned<uint8_t>(std::string_view{"256"}).has_value());
}

TEST(StringTest, ToHexString) {
    ASSERT_EQ("00000001", ToHexString(1));
    ASSERT_EQ("FFFFFFFF", ToHexString(-1));
    ASSERT_EQ("0A", ToHexString(static_cast<uint8_t>(10)));
    ASSERT_EQ("FF", ToHexString(static_cast<uint8_t>(255)));
    ASSERT_EQ("807F", ToHexString(static_cast<uint16_t>(127 + 32768)));
    ASSERT_EQ("7FFF1234567890FF", ToHexString(0x7FFF'1234'5678'90FF));
}

TEST(StringTest, Split) {
    const std::string_view a = "Hello;world!;User;agent;SessionId";
    auto tokens = Split(a, ";");
    ASSERT_EQ(5, tokens.size());
    ASSERT_EQ("Hello",     tokens[0]);
    ASSERT_EQ("world!",    tokens[1]);
    ASSERT_EQ("User",      tokens[2]);
    ASSERT_EQ("agent",     tokens[3]);
    ASSERT_EQ("SessionId", tokens[4]);
}

TEST(StringTest, Split_SingleToken) {
    const std::string_view a = "Hello world!";
    auto tokens = Split(a, ";");
    ASSERT_EQ(1, tokens.size());
    ASSERT_EQ("Hello world!", tokens[0]);
}

TEST(StringTest, Split_MarkerAtTheEnd) {
    const std::string_view a = "Hello world!";
    auto tokens = Split(a, "!");
    ASSERT_EQ(2, tokens.size());
    ASSERT_EQ("Hello world", tokens[0]);
    ASSERT_EQ(std::string_view{}, tokens[1]);
}

TEST(StringTest, Split_IgnoreFirst) {
    const std::string_view a = "Hello world!";
    {
        auto tokens = Split(a, "!", true);
        ASSERT_EQ(1, tokens.size());
        ASSERT_EQ(std::string_view{}, tokens[0]);
    }
    {
        auto tokens = Split(a, " ", true);
        ASSERT_EQ(1, tokens.size());
        ASSERT_EQ("world!", tokens[0]);
    }
}

TEST(StringTest, Split_Empty) {
    {
        const std::string_view a = {};
        auto tokens = Split(a, "!", false);
        ASSERT_EQ(0, tokens.size());
    }
    {
        const std::string_view a = " ";
        auto tokens = Split(a, " ", true);
        ASSERT_EQ(1, tokens.size());
        ASSERT_EQ(std::string_view{}, tokens[0]);
    }
}

TEST(StringTest, Split_Empty2) {
    const std::string_view a = "::token:";
    auto tokens = Split(a, ":");
    ASSERT_EQ(4, tokens.size());
    ASSERT_EQ(std::string_view{}, tokens[0]);
    ASSERT_EQ(std::string_view{}, tokens[1]);
    ASSERT_EQ("token", tokens[2]);
    ASSERT_EQ(std::string_view{}, tokens[3]);
}

TEST(StringTest, ToLowerCase) {
    std::string str = "Hello wOrlD 42!";
    ToLowerCase(str);
    ASSERT_EQ("hello world 42!", str);
}

TEST(StringTest, Prefix) {
    ASSERT_TRUE(IsPrefix("Hello world", "He"));
    ASSERT_FALSE(IsPrefix("Hello world", "wo"));
}

TEST(StringTest, PrefixCaseInsensitive) {
    ASSERT_TRUE(IsPrefix("Hello world", "hElLo", true));
    ASSERT_FALSE(IsPrefix("Hello world", "hElLo"));
}

TEST(StringTest, HexDump) {
    std::array<uint8_t, 8> data = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};
    ASSERT_EQ("12 34 56 78 9A BC DE F0", ToHexDump(data.data(), data.size()));
    ASSERT_EQ("12 34", ToHexDump(data.data(), 2));
    ASSERT_EQ("12", ToHexDump(data.data(), 1));
    ASSERT_EQ("", ToHexDump(data.data(), 0));
}

}
