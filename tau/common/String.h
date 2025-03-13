#pragma once

#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
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

inline std::vector<std::string_view> Split(std::string_view str, std::string_view marker, bool ignore_first = false) {
    size_t prev = 0, pos = 0;
    std::vector<std::string_view> data;
    while((pos = str.find(marker, prev)) != std::string::npos) {
        if(!(ignore_first && (prev == 0))) {
            data.emplace_back(str.substr(prev, pos - prev));
        }
        prev = pos + marker.size();
    }
    if((prev != 0) || !str.empty()) {
        data.emplace_back(str.substr(prev));
    }
    return data;
}

inline std::vector<std::string_view> Split(const std::string& str, std::string_view marker, bool ignore_first = false) {
    return Split(std::string_view(str), marker, ignore_first);
}

inline bool IsPrefix(std::string_view str, std::string_view prefix) {
    return str.find(prefix) == 0;
}

inline bool IsSuffix(std::string_view str, std::string_view prefix) {
    const auto pos = str.find(prefix);
    return (pos != std::string::npos) && (pos + prefix.size() == str.size());
}
