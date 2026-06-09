#include "tau/common/String.h"
#include <etl/char_traits.h>

namespace tau {

etl::vector<etl::string_view, 256> Split(etl::string_view str, etl::string_view marker, bool ignore_first) {
    size_t prev = 0, pos = 0;
    etl::vector<etl::string_view, 256> data;
    while((pos = str.find(marker, prev)) != etl::string_view::npos) {
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

etl::ivector<etl::string_view>& Split(etl::ivector<etl::string_view>& output, etl::string_view str, etl::string_view marker, bool ignore_first) {
    size_t prev = 0, pos = 0;
    while((pos = str.find(marker, prev)) != etl::string_view::npos) {
        if(!(ignore_first && (prev == 0))) {
            output.emplace_back(str.substr(prev, pos - prev));
        }
        prev = pos + marker.size();
    }
    if((prev != 0) || !str.empty()) {
        output.emplace_back(str.substr(prev));
    }
    return output;
}

// std::vector<std::string_view> Split(const std::string& str, std::string_view marker, bool ignore_first) {
//     return Split(std::string_view(str), marker, ignore_first);
// }

// std::string ReplaceAll(std::string str, const std::string& from, const std::string& to) {
//     boost::algorithm::replace_all(str, from, to);
//     return str;
// }

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
