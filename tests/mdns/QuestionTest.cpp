#include "tau/mdns/Question.h"
#include "tests/lib/Common.h"

namespace tau::mdns {

TEST(QuestionTest, Basic) {
    std::vector<uint8_t> data = {
        4, 'T', 'e', 's', 't',
        2, '4', '2',
        5, 'l', 'o', 'c', 'a', 'l',
        0,
        0x12, 0x34,
        0x56, 0x78,
    };
    auto question = ParseQuestion(BufferViewConst{.ptr = data.data(), .size = data.size()});
    ASSERT_TRUE(question.has_value());
    ASSERT_EQ("Test.42.local", question->name);
    ASSERT_EQ(0x1234, question->type);
    ASSERT_EQ(0x5678, question->cash_flush_and_class);
    ASSERT_EQ(data.size(), question->size);
}

TEST(QuestionTest, Malformed) {
    std::vector<uint8_t> data = {
        4, 'T', 'e', 's', 't',
        2, '4', '2',
        5, 'l', 'o', 'c', 'a', 'l',
        0,
        0x12, 0x34,
        0x56,
    };
    auto question = ParseQuestion(BufferViewConst{.ptr = data.data(), .size = data.size()});
    ASSERT_FALSE(question.has_value());
}

}
