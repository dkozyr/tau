#include "tau/common/String.h"
#include "tests/lib/Common.h"

namespace tau {

TEST(StringTest, StringToUnsigned) {
    ASSERT_EQ(1234567890, StringToUnsigned(etl::string_view{"01234567890"}).value());
    ASSERT_EQ(0, StringToUnsigned(etl::string_view{"0"}).value());
    ASSERT_FALSE(StringToUnsigned(etl::string_view{"012345-67890"}).has_value());
    ASSERT_FALSE(StringToUnsigned(etl::string_view{"-12345"}).has_value());
    ASSERT_FALSE(StringToUnsigned(etl::string_view{}).has_value());
    ASSERT_FALSE(StringToUnsigned(etl::string_view{"a"}).has_value());
}

TEST(StringTest, StringToUnsigned_DifferentType) {
    ASSERT_EQ(18446744073709551615ull, StringToUnsigned<uint64_t>(etl::string_view{"18446744073709551615"}).value());
    ASSERT_EQ(4294967296ull, StringToUnsigned<uint64_t>(etl::string_view{"4294967296"}).value());
    ASSERT_EQ(4294967295u, StringToUnsigned<uint32_t>(etl::string_view{"4294967295"}).value());
    ASSERT_FALSE(StringToUnsigned<uint32_t>(etl::string_view{"4294967296"}).has_value());

    ASSERT_EQ(65536, StringToUnsigned<uint64_t>(etl::string_view{"65536"}).value());
    ASSERT_EQ(65536, StringToUnsigned<uint32_t>(etl::string_view{"65536"}).value());
    ASSERT_FALSE(StringToUnsigned<uint16_t>(etl::string_view{"65536"}).has_value());
    ASSERT_FALSE(StringToUnsigned<uint8_t>(etl::string_view{"65536"}).has_value());

    ASSERT_EQ(256, StringToUnsigned<uint16_t>(etl::string_view{"256"}).value());
    ASSERT_EQ(255, StringToUnsigned<uint8_t>(etl::string_view{"255"}).value());
    ASSERT_FALSE(StringToUnsigned<uint8_t>(etl::string_view{"256"}).has_value());
}

TEST(StringTest, ParseStringAsUnsigned) {
    ASSERT_EQ(1234567890, ParseStringAsUnsigned(etl::string_view{"01234567890"}));
    ASSERT_EQ(0, ParseStringAsUnsigned(etl::string_view{"0"}));
    ASSERT_EQ(12345, ParseStringAsUnsigned(etl::string_view{"012345-67890"}));
    ASSERT_EQ(0, ParseStringAsUnsigned(etl::string_view{"-12345"}));
    ASSERT_EQ(0, ParseStringAsUnsigned(etl::string_view{}));
    ASSERT_EQ(0, ParseStringAsUnsigned(etl::string_view{"a42"}));
}

TEST(StringTest, ParseStringAsFloat) {
    ASSERT_EQ(1234567890.0f, ParseStringAsFloat(etl::string_view{"01234567890"}));
    ASSERT_EQ(123456.7890f, ParseStringAsFloat(etl::string_view{"0123456.7890"}));
    ASSERT_EQ(123.456f, ParseStringAsFloat(etl::string_view{"0123.456.7890"}));
    ASSERT_EQ(0, ParseStringAsFloat(etl::string_view{"0"}));
    ASSERT_EQ(12345.0f, ParseStringAsFloat(etl::string_view{"012345-67890"}));
    ASSERT_EQ(0, ParseStringAsFloat(etl::string_view{"-12345"}));
    ASSERT_EQ(0, ParseStringAsFloat(etl::string_view{}));
    ASSERT_EQ(0, ParseStringAsFloat(etl::string_view{"a42"}));
}

TEST(StringTest, ToHexString) {
    ASSERT_STREQ("00000001", ToHexString(1).c_str());
    ASSERT_STREQ("FFFFFFFF", ToHexString(-1).c_str());
    ASSERT_STREQ("0A", ToHexString(static_cast<uint8_t>(10)).c_str());
    ASSERT_STREQ("FF", ToHexString(static_cast<uint8_t>(255)).c_str());
    ASSERT_STREQ("807F", ToHexString(static_cast<uint16_t>(127 + 32768)).c_str());
    ASSERT_STREQ("7FFF1234567890FF", ToHexString(0x7FFF'1234'5678'90FF).c_str());

    ASSERT_STREQ("00000001", ToHexString<false>(1).c_str());
    ASSERT_STREQ("ffffffff", ToHexString<false>(-1).c_str());
    ASSERT_STREQ("0a", ToHexString<false>(static_cast<uint8_t>(10)).c_str());
    ASSERT_STREQ("ff", ToHexString<false>(static_cast<uint8_t>(255)).c_str());
    ASSERT_STREQ("807f", ToHexString<false>(static_cast<uint16_t>(127 + 32768)).c_str());
    ASSERT_STREQ("7fff1234567890ff", ToHexString<false>(0x7FFF'1234'5678'90FF).c_str());
}

TEST(StringTest, Split) {
    const etl::string_view a = "Hello;world!;User;agent;SessionId";
    SplitTokens<8> tokens;
    Split(tokens, a, ";");
    ASSERT_EQ(5, tokens.size());
    ASSERT_EQ("Hello",     tokens[0]);
    ASSERT_EQ("world!",    tokens[1]);
    ASSERT_EQ("User",      tokens[2]);
    ASSERT_EQ("agent",     tokens[3]);
    ASSERT_EQ("SessionId", tokens[4]);
}

TEST(StringTest, Split_SmallCapacity) {
    const etl::string_view a = "Hello;world!;User;agent;SessionId";
    SplitTokens<1> tokens;
    Split(tokens, a, ";");
    ASSERT_EQ(2, tokens.size());
    ASSERT_EQ("Hello",     tokens[0]);
    ASSERT_EQ("world!",    tokens[1]);
}

TEST(StringTest, Split_SingleToken) {
    const etl::string_view a = "Hello world!";
    SplitTokens<2> tokens;
    Split(tokens, a, ";");
    ASSERT_EQ(1, tokens.size());
    ASSERT_EQ("Hello world!", tokens[0]);
}

TEST(StringTest, Split_MarkerAtTheEnd) {
    const etl::string_view a = "Hello world!";
    SplitTokens<3> tokens;
    Split(tokens, a, "!");
    ASSERT_EQ(2, tokens.size());
    ASSERT_EQ("Hello world", tokens[0]);
    ASSERT_EQ(11, tokens[0].size());
    ASSERT_TRUE(tokens[1].empty());
}

TEST(StringTest, Split_IgnoreFirst) {
    const etl::string_view a = "Hello world!";
    {
        SplitTokens<2> tokens;
        Split(tokens, a, "!", true);
        ASSERT_EQ(1, tokens.size());
        ASSERT_TRUE(tokens[0].empty());
    }
    {
        SplitTokens<2> tokens;
        Split(tokens, a, " ", true);
        ASSERT_EQ(1, tokens.size());
        ASSERT_EQ("world!", tokens[0]);
        ASSERT_EQ(6, tokens[0].size());
    }
}

TEST(StringTest, Split_Empty) {
    {
        const etl::string_view a = {};
        SplitTokens<2> tokens;
        Split(tokens, a, "!", false);
        ASSERT_EQ(1, tokens.size());
        ASSERT_TRUE(tokens[0].empty());
    }
    {
        const etl::string_view a = {};
        SplitTokens<2> tokens;
        Split(tokens, a, "!", true);
        ASSERT_EQ(0, tokens.size());
    }
    {
        const etl::string_view a = " ";
        SplitTokens<2> tokens;
        Split(tokens, a, " ", true);
        ASSERT_EQ(1, tokens.size());
        ASSERT_TRUE(tokens[0].empty());
    }
}

TEST(StringTest, Split_Empty2) {
    const etl::string_view a = "::token:";
    SplitTokens<8> tokens;
    Split(tokens, a, ":");
    ASSERT_EQ(4, tokens.size());
    ASSERT_TRUE(tokens[0].empty());
    ASSERT_TRUE(tokens[1].empty());
    ASSERT_EQ("token", tokens[2]);
    ASSERT_EQ(5, tokens[2].size());
    ASSERT_TRUE(tokens[3].empty());
}

TEST(StringTest, ReplaceAll) {
    etl::string<64> output;

    etl::string_view input = "Hello world! Welcome to the world of C++.";
    ReplaceAll(output, input, "world", "universe");
    ASSERT_EQ("Hello universe! Welcome to the universe of C++.", output);

    ReplaceAll(output, "Hello World! 42", " ", "");
    ASSERT_EQ("HelloWorld!42", output);

    ReplaceAll(output, "Hello World! 42", " ", "  ");
    ASSERT_EQ("Hello  World!  42", output);

    ReplaceAll(output, "aaaaaaa", "aa", "b");
    ASSERT_EQ("bbba", output);

    ReplaceAll(output, "No matches here.", "xyz", "123");
    ASSERT_EQ("No matches here.", output);
}

TEST(StringTest, ToLowerCase) {
    etl::string<16> str = "Hello wOrlD 42!";
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
    etl::array<uint8_t, 8> data = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};
    etl::string<256> dump;
    ASSERT_STREQ("12 34 56 78 9A BC DE F0", ToHexDump(data.data(), data.size(), dump).c_str());
    ASSERT_STREQ("12 34", ToHexDump(data.data(), 2, dump).c_str());
    ASSERT_STREQ("12", ToHexDump(data.data(), 1, dump).c_str());
    ASSERT_STREQ("", ToHexDump(data.data(), 0, dump).c_str());

    ASSERT_STREQ("123456789ABCDEF0", ToHexDump(data.data(), data.size(), dump, {}).c_str());
    ASSERT_STREQ("123456789abcdef0", ToHexDump<false>(data.data(), data.size(), dump, {}).c_str());
    ASSERT_STREQ("12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0", ToHexDump<false>(data.data(), data.size(), dump, ", 0x").c_str());
}

TEST(StringTest, EtlStringTruncation) {
    etl::string_view src = "0123456789";

    etl::string<8> dest;
    dest = src;
    ASSERT_EQ("01234567", dest);
}

}
