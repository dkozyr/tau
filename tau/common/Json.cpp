#include <tau/common/Json.h>

namespace tau::json {

std::string GetString(const Json::value& json, const std::string& key) {
    if(!json.as_object().contains(key) || !json.at(key).is_string()) {
        return {};
    }
    auto value = json.at(key).get_string();
    return std::string(value.data(), value.size());
}

double GetDouble(const Json::value& json, const std::string& key) {
    if(json.as_object().contains(key)) {
        if(json.at(key).is_double()) {
            return json.at(key).get_double();
        }
        if(json.at(key).is_int64()) {
            return json.at(key).get_int64();
        }
    }
    return 0;
}

double GetDoubleFromString(const Json::value& json, const std::string& key) {
    auto value = GetString(json, key);
    if(!value.empty()) {
        return std::stod(value);
    }
    return 0;
}

}
