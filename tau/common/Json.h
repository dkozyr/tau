#pragma once

#include <boost/json.hpp>
#include <etl/string.h>
#include <etl/string_stream.h>
#include <etl/string_view.h>

namespace tau {

namespace Json = boost::json;
using boost_ec = boost::system::error_code;

}

namespace tau::json {

etl::istring& Serialize(const Json::object& object, etl::istring& output);
etl::string_stream& SerializeString(etl::string_stream& ss, const etl::string_view& value);

etl::istring& GetString(const Json::value& json, const etl::string_view& key, etl::istring& output);
etl::istring& GetString(const Json::value& json, etl::istring& output);
etl::string_view GetStringView(const Json::value& json, const etl::string_view& key);
etl::string_view GetStringView(const Json::value& json);
double GetDouble(const Json::value& json, const etl::string_view& key);
uint64_t GetUint64(const Json::value& json, const etl::string_view& key);
double GetDoubleFromString(const Json::value& json, const etl::string_view& key);

}
