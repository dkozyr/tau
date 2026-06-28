#pragma once

#include <etl/string.h>
#include <etl/string_view.h>
#include <string>
#include <string_view>
#include <sstream>

namespace tau {

template<typename T>
std::string ToStdString(const T& value) {
    std::stringstream ss;
    ss << value;
    return ss.str();
}

template<typename T>
std::string_view ToStdStringView(const T& str) {
    return std::string_view{str.data(), str.size()};
}

}
