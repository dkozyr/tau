#include <tau/common/Json.h>

namespace tau::json {

etl::istring& GetString(const Json::value& json, const etl::string_view& key, etl::istring& output) {
    output.clear();
    if(!json.as_object().contains(key.data()) || !json.at(key.data()).is_string()) {
        return output;
    }
    const auto& value = json.at(key.data()).get_string();
    output.append(value.data(), value.size());
    return output;
}

etl::string_view GetStringView(const Json::value& json, const etl::string_view& key) {
    if(!json.as_object().contains(key.data()) || !json.at(key.data()).is_string()) {
        return {};
    }
    const auto& value = json.at(key.data()).get_string();
    return etl::string_view{value.data(), value.size()};
}

etl::string_view GetStringView(const Json::value& value) {
    if(!value.is_string()) {
        return {};
    }
    const auto& value_str = value.get_string();
    return etl::string_view{value_str.data(), value_str.size()};
}

double GetDouble(const Json::value& json, const etl::string_view& key) {
    if(json.as_object().contains(key.data())) {
        if(json.at(key.data()).is_double()) {
            return json.at(key.data()).get_double();
        }
        if(json.at(key.data()).is_int64()) {
            return json.at(key.data()).get_int64();
        }
    }
    return 0;
}

double GetDoubleFromString(const Json::value& json, const etl::string_view& key) {
    auto value = GetStringView(json, key);
    if(!value.empty()) {
        return std::stod(value.data());
    }
    return 0;
}

}
