#pragma once

#include "tau/common/Math.h"
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/algorithm/string.hpp>
#include <string_view>

namespace tau {

// Idea from StackOverflow: https://stackoverflow.com/a/28471421/2210772
inline std::string Base64Decode(std::string_view str) {
    using namespace boost::archive::iterators;
    using It = transform_width<binary_from_base64<std::string::const_iterator>, 8, 6>;
    return boost::algorithm::trim_right_copy_if(std::string(It(std::begin(str)), It(std::end(str))),
        [](char c) {
            return c == '\0';
        });
}

inline std::string Base64Encode(std::string_view data) {
    using namespace boost::archive::iterators;
    using It = base64_from_binary<transform_width<std::string::const_iterator, 6, 8>>;
    auto result = std::string(It(std::begin(data)), It(std::end(data)));
    return result.append(Align(data.size(), 3) - data.size(), '=');
}

inline std::string Base64Encode(void* data, size_t size) {
    return Base64Encode(std::string_view{reinterpret_cast<const char*>(data), size});
}

}
