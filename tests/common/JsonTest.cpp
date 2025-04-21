#include "tau/common/Json.h"
#include "tests/lib/Common.h"

namespace tau {

TEST(JsonTest, Basic) {
    Json::object object = {
        {"pi", 3.14},
        {"hello", "world"},
        {"nothing", nullptr},
        {"answer", {
            {"everything", 42}
        }},
        {"list", {1, 1, 2, 3, 5, 8, 13, 21}}
    };
    auto serialized = Json::serialize(object);
    ASSERT_EQ(serialized, R"({"pi":3.14E0,"hello":"world","nothing":null,"answer":{"everything":42},"list":[1,1,2,3,5,8,13,21]})");

    boost_ec ec;
    auto parsed = Json::parse(serialized, ec);
    ASSERT_EQ(0, ec.value());

    ASSERT_NEAR(3.14, object.at("pi").get_double(), 1e-5);
    ASSERT_EQ("world", object.at("hello").get_string());
    ASSERT_TRUE(object.find("nothing") != object.end());
    ASSERT_EQ(42, object.at("answer").at("everything").get_int64());
    auto& list = object.at("list").get_array();
    ASSERT_EQ(8, list.size());
    size_t fibo_prev = 1;
    size_t fibo = 0;
    for(auto i = 0; i < 8; ++i) {
        fibo_prev += fibo;
        std::swap(fibo, fibo_prev);
        ASSERT_EQ(fibo, list.at(i).get_int64());
    }
}

}
