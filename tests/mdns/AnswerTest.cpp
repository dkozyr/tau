#include "tau/mdns/Answer.h"
#include "tests/lib/Common.h"

namespace tau::mdns {

TEST(AnswerTest, Basic) {
    std::vector<uint8_t> data = {
        4, 'T', 'e', 's', 't',
        2, '4', '2',
        5, 'l', 'o', 'c', 'a', 'l',
        0,
        0x12, 0x34,
        0x56, 0x78,
        0x9A, 0xBC, 0xDE, 0xF0,
        0, 7,
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
    };
    auto answer = ParseAnswer(BufferViewConst{.ptr = data.data(), .size = data.size()});
    ASSERT_TRUE(answer.has_value());
    ASSERT_EQ("Test.42.local", answer->name);
    ASSERT_EQ(0x1234, answer->type);
    ASSERT_EQ(0x5678, answer->cash_flush_and_class);
    ASSERT_EQ(0x9ABCDEF0, answer->ttl);
    ASSERT_EQ(7, answer->data.size);
    ASSERT_EQ(data.data() + data.size(), answer->data.ptr + answer->data.size);
    ASSERT_EQ(0, std::memcmp(data.data() + data.size() - 7, answer->data.ptr, answer->data.size));
}

TEST(AnswerTest, EmptyData) {
    std::vector<uint8_t> data = {
        4, 'T', 'e', 's', 't',
        2, '4', '2',
        5, 'l', 'o', 'c', 'a', 'l',
        0,
        0x12, 0x34,
        0x56, 0x78,
        0x9A, 0xBC, 0xDE, 0xF0,
        0, 0,
    };
    auto answer = ParseAnswer(BufferViewConst{.ptr = data.data(), .size = data.size()});
    ASSERT_TRUE(answer.has_value());
    ASSERT_EQ("Test.42.local", answer->name);
    ASSERT_EQ(0x1234, answer->type);
    ASSERT_EQ(0x5678, answer->cash_flush_and_class);
    ASSERT_EQ(0x9ABCDEF0, answer->ttl);
    ASSERT_EQ(0, answer->data.size);
    ASSERT_EQ(data.data() + data.size(), answer->data.ptr);
}

TEST(AnswerTest, Malformed) {
    std::vector<uint8_t> data = {
        4, 'T', 'e', 's', 't',
        2, '4', '2',
        5, 'l', 'o', 'c', 'a', 'l',
        0,
        0x12, 0x34,
        0x56, 0x78,
        0x9A, 0xBC, 0xDE, 0xF0,
        0, 3,
        0x01, 0x02
    };
    auto answer = ParseAnswer(BufferViewConst{.ptr = data.data(), .size = data.size()});
    ASSERT_FALSE(answer.has_value());
}

TEST(AnswerTest, Malformed2) {
    std::vector<uint8_t> data = {
        4, 'T', 'e', 's', 't',
        2, '4', '2',
        5, 'l', 'o', 'c', 'a', 'l',
        0,
        0x12,
    };
    auto answer = ParseAnswer(BufferViewConst{.ptr = data.data(), .size = data.size()});
    ASSERT_FALSE(answer.has_value());
}

}
