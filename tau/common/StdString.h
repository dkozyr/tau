#pragma once

#include <string>
#include <sstream>

namespace tau {

template<typename T>
std::string ToStdString(const T& value) {
    std::stringstream ss;
    ss << value;
    return ss.str();
}

}
