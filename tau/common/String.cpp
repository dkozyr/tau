#include "tau/common/String.h"
#include <etl/char_traits.h>

namespace tau {

etl::ivector<etl::string_view>& Split(etl::ivector<etl::string_view>& output, etl::string_view str, etl::string_view marker, bool ignore_first) {
    size_t pos = 0;
    while((pos != etl::string_view::npos) && !output.full()) {
        auto token = SplitNext(str, pos, marker);
        if(!ignore_first) {
            output.emplace_back(token);
        } else {
            ignore_first = false;
        }
    }
    return output;
}

etl::string_view SplitNext(etl::string_view str, size_t& prev, etl::string_view marker) {
    auto pos = str.find(marker, prev);
    if(pos != etl::string_view::npos) {
        auto result = str.substr(prev, pos - prev);
        prev = pos + marker.size();
        return result;
    } else {
        auto result = str.substr(prev);
        prev = pos;
        return result;
    }
}

void ReplaceAll(etl::istring& output, etl::string_view input, etl::string_view from, etl::string_view to) {
    output.clear();
    size_t prev = 0, pos = 0;
    while((pos = input.find(from, prev)) != etl::string_view::npos) {
        output.append(input.substr(prev, pos - prev));
        output.append(to);
        prev = pos + from.size();
    }
    if((prev != 0) || !input.empty()) {
        output.append(input.substr(prev));
    }
}

void ToLowerCase(etl::istring& value) {
    for(auto& c : value) {
        c = ToLower(c);
    }
}

bool IsPrefix(etl::string_view str, etl::string_view prefix, bool case_insensitive) {
    if(case_insensitive) {
        if(str.empty() || prefix.empty() || (str.size() < prefix.size())) {
            return false;
        }
        for(size_t i = 0; i < prefix.size(); ++i) {
            if(ToUpper(str[i]) != ToUpper(prefix[i])) {
                return false;
            }
        }
        return true;
    } else {
        return str.find(prefix) == 0;
    }
}

bool IsAlphaDigit(char c) {
    return IsDigit(c) || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

bool IsDigit(char c) {
    return (c >= '0') && (c <= '9');
}

char ToUpper(char c) {
    return (c >= 'a' && c <= 'z') ? (c - ('a' - 'A')) : c;
}

char ToLower(char c) {
    return (c >= 'A' && c <= 'Z') ? (c - ('A' - 'a')) : c;
}

}
