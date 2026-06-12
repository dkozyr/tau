#pragma once

#include <etl/string.h>
#include <etl/string_view.h>
#include <etl/string_stream.h>
#include <etl/vector.h>
#include <optional>
#include <limits>
#include <cstring>
#include <cctype>
#include <cstdint>
#include <cstddef>
#include <type_traits>

namespace tau {

template<size_t ExpectedCapacity>
using SplitTokens = etl::vector<etl::string_view, ExpectedCapacity + 1>;

//TODO: remove with fixed capacity
etl::vector<etl::string_view, 256> Split(etl::string_view str, etl::string_view marker, bool ignore_first = false);
etl::ivector<etl::string_view>& Split(etl::ivector<etl::string_view>& output, etl::string_view str, etl::string_view marker, bool ignore_first = false);
etl::string_view SplitNext(etl::string_view str, size_t& pos, etl::string_view marker);

void ReplaceAll(etl::istring& output, etl::string_view input, etl::string_view from, etl::string_view to);
void ToLowerCase(etl::istring& value);
bool IsPrefix(etl::string_view str, etl::string_view prefix, bool case_insensitive = false);
bool IsAlphaDigit(char c);
bool IsDigit(char c);
char ToUpper(char c);
char ToLower(char c);

template<typename R = size_t, typename T>
std::optional<R> StringToUnsigned(const T& str) {
    if(str.empty()) {
        return std::nullopt;
    }
    uint64_t value = 0;
    for(auto& c : str) {
        if(!IsDigit(c)) {
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
        if(!IsDigit(c)) {
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
        if(IsDigit(c)) {
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

template<bool UpperCase = true, typename T = size_t, size_t N = 2 * sizeof(T)>
etl::string<N> ToHexString(T value) {
    static_assert(std::is_integral_v<T>);
    const etl::string_view kHexData = UpperCase ? "0123456789ABCDEF" : "0123456789abcdef";
    etl::string<N> result(N, '0');
    for(size_t i = 0; (i < N) && value; ++i) {
        result[N - 1 - i] = kHexData[value & 0x0F];
        value >>= 4;
    }
    return result;
}

// template<typename T>
// std::string ToString(const T& value) {
//     std::stringstream ss;
//     ss << value;
//     return ss.str();
// }

template<bool UpperCase = true>
etl::istring& ToHexDump(const uint8_t* ptr, size_t size, etl::istring& output, etl::string_view separator = " ") {
    output.clear();
    for(size_t i = 0; i < size; ++i) {
        if(i > 0 && !separator.empty()) {
            output += separator;
        }
        output += ToHexString<UpperCase>(ptr[i]);
    }
    return output;
}

}
