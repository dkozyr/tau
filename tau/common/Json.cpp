#include <tau/common/Json.h>

namespace tau::json {

etl::istring& GetString(const Json::value& json, const etl::string_view& key, etl::istring& output) {
    output.clear();
    if(!json.as_object().contains(key.data()) || !json.at(key.data()).is_string()) {
        return output;
    }
    auto value = json.at(key.data()).get_string();
    output.append(value.data(), value.size());
    return output;
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
    etl::string<16> value;
    GetString(json, key, value);
    if(!value.empty()) {
        return std::stod(value.data());
    }
    return 0;
}

}
