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
    etl::string<8> output;
    ASSERT_EQ("world", GetString(kObject, "hello", output));
    ASSERT_EQ("", GetString(kObject, "pi", output));
    ASSERT_EQ("", GetString(kObject, "nothing", output));
    ASSERT_EQ("", GetString(kObject, "answer", output));

    ASSERT_EQ("world", GetString(kObject.at("hello"), output));
    ASSERT_TRUE(GetString(kObject.at("pi"), output).empty());
    ASSERT_TRUE(GetString(kObject.at("nothing"), output).empty());
    ASSERT_TRUE(GetString(kObject.at("answer"), output).empty());
}

TEST(JsonTest, GetStringView) {
    ASSERT_EQ("world", GetStringView(kObject, "hello"));
    ASSERT_TRUE(GetStringView(kObject, "pi").empty());
    ASSERT_TRUE(GetStringView(kObject, "nothing").empty());
    ASSERT_TRUE(GetStringView(kObject, "answer").empty());

    ASSERT_EQ("world", GetStringView(kObject.at("hello")));
    ASSERT_TRUE(GetStringView(kObject.at("pi")).empty());
    ASSERT_TRUE(GetStringView(kObject.at("nothing")).empty());
    ASSERT_TRUE(GetStringView(kObject.at("answer")).empty());
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
