#pragma once

#include <boost/json.hpp>
#include <string>

namespace tau {

namespace Json = boost::json;
using boost_ec = boost::system::error_code;

}

namespace tau::json {

std::string GetString(const Json::value& json, const std::string& key);
double GetDouble(const Json::value& json, const std::string& key);
double GetDoubleFromString(const Json::value& json, const std::string& key);

}
