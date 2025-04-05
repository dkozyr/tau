#pragma once

#include <string>
#include <string_view>
#include <sstream>
#include <vector>
#include <optional>
#include <limits>
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

template<typename T = size_t>
std::string ToHexString(T value) {
    static_assert(std::is_integral_v<T>);
    const std::string_view kHexData = "0123456789ABCDEF";
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
void ToLowerCase(std::string& value);
bool IsPrefix(std::string_view str, std::string_view prefix, bool case_insensitive = false);
std::string ToHexDump(const uint8_t* ptr, size_t size);

}
