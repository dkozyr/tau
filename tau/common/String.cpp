#include "tau/common/String.h"

namespace tau {

std::vector<std::string_view> Split(std::string_view str, std::string_view marker, bool ignore_first) {
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

std::vector<std::string_view> Split(const std::string& str, std::string_view marker, bool ignore_first) {
    return Split(std::string_view(str), marker, ignore_first);
}

bool IsPrefix(std::string_view str, std::string_view prefix, bool case_insensitive) {
    if(case_insensitive) {
        if((str.size() < prefix.size()) || prefix.empty()) {
            return false;
        }
        for(size_t i = 0; i < prefix.size(); ++i) {
            if(std::toupper(str[i]) != std::toupper(prefix[i])) {
                return false;
            }
        }
        return true;
    } else {
        return str.find(prefix) == 0;
    }
}

}
