#pragma once

#include <string>
#include <string_view>
#include <sstream>
#include <vector>
#include <optional>
#include <cctype>
#include <type_traits>

namespace tau {

template<typename R = size_t, typename T>
std::optional<R> StringToUnsigned(const T& str) {
    std::optional<R> result;
    for(auto& c : str) {
        if(!std::isdigit(c)) {
            break;
        }
        result = result.value_or(0) * 10 + (c - '0');
    }
    return result;
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

std::vector<std::string_view> Split(std::string_view str, std::string_view marker, bool ignore_first = false);
std::vector<std::string_view> Split(const std::string& str, std::string_view marker, bool ignore_first = false);
bool IsPrefix(std::string_view str, std::string_view prefix, bool case_insensitive = false);

}
