#pragma once

#include <boost/json.hpp>
#include <etl/string.h>
#include <etl/string_view.h>

namespace tau {

namespace Json = boost::json;
using boost_ec = boost::system::error_code;

}

namespace tau::json {

etl::istring& GetString(const Json::value& json, const etl::string_view& key, etl::istring& output);
etl::string_view GetStringView(const Json::value& json, const etl::string_view& key);
double GetDouble(const Json::value& json, const etl::string_view& key);
double GetDoubleFromString(const Json::value& json, const etl::string_view& key);

}
