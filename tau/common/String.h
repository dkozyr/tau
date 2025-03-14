#pragma once

#include <string>
#include <string_view>
#include <sstream>
#include <vector>
#include <optional>

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
    const std::string_view kHexData = "0123456789ABCDEF";
    std::string result;
    result.reserve(16);
    while(value > 0) {
        result = kHexData[value & 0x0F] + result;
        value >>= 4;
    }
    return result.empty() ? "0" : result;
}

std::vector<std::string_view> Split(std::string_view str, std::string_view marker, bool ignore_first = false);
std::vector<std::string_view> Split(const std::string& str, std::string_view marker, bool ignore_first = false);
bool IsPrefix(std::string_view str, std::string_view prefix, bool case_insensitive = false);
