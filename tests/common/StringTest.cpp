#include "tau/common/String.h"
#include "tests/Common.h"

TEST(StringTest, StringToUnsigned) {
    ASSERT_EQ(1234567890, StringToUnsigned(std::string_view{"1234567890"}));
    ASSERT_EQ(12345, StringToUnsigned(std::string_view{"012345-67890"}));
    ASSERT_EQ(0, StringToUnsigned(std::string_view{"0"}));
    ASSERT_FALSE(StringToUnsigned(std::string_view{"-12345"}).has_value());
    ASSERT_FALSE(StringToUnsigned(std::string_view{}).has_value());
    ASSERT_FALSE(StringToUnsigned(std::string_view{"a"}).has_value());
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

TEST(StringTest, Prefix) {
    ASSERT_TRUE(IsPrefix("Hello world", "He"));
    ASSERT_FALSE(IsPrefix("Hello world", "wo"));
}

TEST(StringTest, PrefixCaseInsensitive) {
    ASSERT_TRUE(IsPrefix("Hello world", "hElLo", true));
    ASSERT_FALSE(IsPrefix("Hello world", "hElLo"));
}
