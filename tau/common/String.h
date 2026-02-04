#pragma once

#include <string>
#include <string_view>
#include <sstream>
#include <vector>
#include <optional>
#include <limits>
#include <cstring>
#include <cctype>
#include <cstdint>
#include <cstddef>
#include <type_traits>

namespace tau {

template<typename R = size_t, typename T>
std::optional<R> StringToUnsigned(const T& str) {
    if(str.empty()) {
        return std::nullopt;
    }
    uint64_t value = 0;
    for(auto& c : str) {
        if(!std::isdigit(c)) {
            return std::nullopt;
        }
        value = value * 10 + (c - '0');
    }
    if(value > std::numeric_limits<R>::max()) {
        return std::nullopt;
    }
    return static_cast<R>(value);
}

template<typename R = size_t, typename T>
R ParseStringAsUnsigned(const T& str) {
    R value = 0;
    for(auto& c : str) {
        if(!std::isdigit(c)) {
            break;
        }
        value = value * 10 + (c - '0');
    }
    return value;
}

template<typename R = float, typename T>
R ParseStringAsFloat(const T& str) {
    uint64_t integer = 0;
    size_t i = 0;
    while(i < str.size()) {
        auto c = str[i];
        i++;
        if(std::isdigit(c)) {
            integer = integer * 10 + (c - '0');
        } else if(c == '.') {
            break;
        } else {
            return static_cast<R>(integer);
        }
    }
    R value = static_cast<R>(integer);
    auto base = static_cast<R>(0.1);
    while(i < str.size()) {
        auto c = str[i];
        i++;
        if(std::isdigit(c)) {
            value += (c - '0') * base;
            base *= static_cast<R>(0.1);
        } else {
            break;
        }
    }
    return value;
}

template<bool UpperCase = true, typename T = size_t>
std::string ToHexString(T value) {
    static_assert(std::is_integral_v<T>);
    const std::string_view kHexData = UpperCase ? "0123456789ABCDEF" : "0123456789abcdef";
    constexpr size_t kStringSize = 2 * sizeof(T);
    std::string result(kStringSize, '0');
    for(size_t i = 0; (i < kStringSize) && value; ++i) {
        result[kStringSize - 1 - i] = kHexData[value & 0x0F];
        value >>= 4;
    }
    return result;
}

template<typename T>
std::string ToString(const T& value) {
    std::stringstream ss;
    ss << value;
    return ss.str();
}

std::vector<std::string_view> Split(std::string_view str, std::string_view marker, bool ignore_first = false);
std::vector<std::string_view> Split(const std::string& str, std::string_view marker, bool ignore_first = false);
std::string ReplaceAll(std::string str, const std::string& from, const std::string& to);
void ToLowerCase(std::string& value);
bool IsPrefix(std::string_view str, std::string_view prefix, bool case_insensitive = false);
std::string ToHexDump(const uint8_t* ptr, size_t size, std::string_view separator = {});

}
