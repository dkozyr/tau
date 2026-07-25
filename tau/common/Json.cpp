#include <tau/common/Json.h>
#include <etl/string_stream.h>

namespace tau::json {

etl::string_stream& Serialize(etl::string_stream& ss, const Json::object& object);
etl::string_stream& Serialize(etl::string_stream& ss, const Json::value& value);

etl::istring& Serialize(const Json::object& object, etl::istring& output) {
    etl::string_stream ss(output);
    Serialize(ss, object);
    return output;
}

etl::istring& GetString(const Json::value& json, const etl::string_view& key, etl::istring& output) {
    output.clear();
    if(!json.as_object().contains(key.data()) || !json.at(key.data()).is_string()) {
        return output;
    }
    const auto& value = json.at(key.data()).get_string();
    output.append(value.data(), value.size());
    return output;
}

etl::istring& GetString(const Json::value& value, etl::istring& output) {
    output.clear();
    if(!value.is_string()) {
        return output;
    }
    const auto& value_str = value.get_string();
    output.append(value_str.data(), value_str.size());
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
        const auto& value = json.at(key.data());
        if(value.is_double()) { return value.get_double(); }
        if(value.is_uint64()) { return value.get_uint64(); }
        if(value.is_int64()) { return value.get_int64(); }
    }
    return 0;
}

uint64_t GetUint64(const Json::value& json, const etl::string_view& key) {
    if(json.as_object().contains(key.data())) {
        const auto& value = json.at(key.data());
        if(value.is_uint64()) { return value.get_uint64(); }
        if(value.is_int64()) { return value.get_int64(); }
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

etl::string_stream& SerializeString(etl::string_stream& ss, const etl::string_view& value) {
    ss << "\"";
    for(size_t i = 0; i < value.size(); ++i) {
        switch(value[i]) {
            case '"': ss << R"(\")"; break;
            case '\\': ss << R"(\\)"; break;
            case '\b': ss << R"(\b)"; break;
            case '\f': ss << R"(\f)"; break;
            case '\n': ss << R"(\n)"; break;
            case '\r': ss << R"(\r)"; break;
            case '\t': ss << R"(\t)"; break;
            default: ss << value.substr(i, 1); break;
        }
    }
    ss << "\"";
    return ss;
}

etl::string_stream& Serialize(etl::string_stream& ss, const Json::object& object) {
    ss << "{";
    bool first = true;
    for(const auto& [key, value] : object) {
        if(!first) {
            ss << ",";
        }
        first = false;
        ss << "\"" << key.data() << "\":";

        if(value.is_object()) {
            Serialize(ss, value.as_object());
        } else {
            Serialize(ss, value);
        }
    }
    ss << "}";
    return ss;
}

etl::string_stream& Serialize(etl::string_stream& ss, const Json::value& value) {
    if(value.is_double()) {
        ss << etl::setprecision(6) << value.get_double();
    } else if(value.is_uint64()) { ss << value.get_uint64();
    } else if(value.is_int64()) { ss << value.get_int64();
    } else if(value.is_string()) {
        const auto& string_value = value.get_string();
        SerializeString(ss, etl::string_view{string_value.data(), string_value.size()});
    } else if(value.is_null()) {
        ss << "null";
    } else if(value.is_array()) {
        ss << "[";
        bool first = true;
        for(auto& x : value.get_array()) {
            if(!first) {
                ss << ",";
            }
            first = false;
            Serialize(ss, x);
        }
        ss << "]";
    } else {
        ss << "null";
    }
    return ss;
}

}
