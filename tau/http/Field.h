#pragma once

#include "tau/asio/Common.h"
#include <string>
#include <variant>
#include <vector>

namespace tau::http {

struct Field {
    using Name = std::variant<beast_http::field, std::string>;

    Name name;
    std::string value;
};

using Fields = std::vector<Field>;

}
