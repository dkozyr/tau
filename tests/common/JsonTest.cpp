#include "tau/common/Json.h"
#include "tests/lib/Common.h"

namespace tau::json {

const Json::object kObject = {
    {"pi", 3.14},
    {"int", 42},
    {"hello", "world"},
    {"nothing", nullptr},
    {"answer", {
        {"everything", 42}
    }},
    {"list", {1, 1, 2, 3, 5, 8, 13, 21}}
};

TEST(JsonTest, Basic) {
    auto serialized = Json::serialize(kObject);
    ASSERT_EQ(serialized, R"({"pi":3.14E0,"int":42,"hello":"world","nothing":null,"answer":{"everything":42},"list":[1,1,2,3,5,8,13,21]})");

    boost_ec ec;
    auto parsed = Json::parse(serialized, ec);
    ASSERT_EQ(0, ec.value());

    ASSERT_NEAR(3.14, kObject.at("pi").get_double(), 1e-5);
    ASSERT_EQ("world", kObject.at("hello").get_string());
    ASSERT_TRUE(kObject.find("nothing") != kObject.end());
    ASSERT_EQ(42, kObject.at("answer").at("everything").get_int64());
    auto& list = kObject.at("list").get_array();
    ASSERT_EQ(8, list.size());
    size_t fibo_prev = 1;
    size_t fibo = 0;
    for(auto i = 0; i < 8; ++i) {
        fibo_prev += fibo;
        std::swap(fibo, fibo_prev);
        ASSERT_EQ(fibo, list.at(i).get_int64());
    }
}

TEST(JsonTest, GetString) {
    ASSERT_EQ("world", GetString(kObject, "hello"));
    ASSERT_EQ("", GetString(kObject, "pi"));
    ASSERT_EQ("", GetString(kObject, "nothing"));
    ASSERT_EQ("", GetString(kObject, "answer"));
}

TEST(JsonTest, GetDouble) {
    const double kEps = 1e-5;
    ASSERT_NEAR(3.14, GetDouble(kObject, "pi"), kEps);
    ASSERT_NEAR(42.0, GetDouble(kObject, "int"), kEps);
    ASSERT_NEAR(0.0, GetDouble(kObject, "nothing"), kEps);
    ASSERT_NEAR(0.0, GetDouble(kObject, "answer"), kEps);
}

TEST(JsonTest, GetDoubleFromString) {
    const Json::object object = {
        {"pi", 3.14},
        {"e", "2.71828"},
    };

    const double kEps = 1e-5;
    ASSERT_NEAR(0.0, GetDoubleFromString(object, "pi"), kEps);
    ASSERT_NEAR(2.71828, GetDoubleFromString(object, "e"), kEps);
}

}
