#pragma once

#include "tau/asio/Common.h"
#include "tau/common/Variant.h"
#include <etl/vector.h>
#include <etl/string_view.h>

namespace tau::http {

struct Field {
    using Name = std::variant<beast_http::field, etl::string_view>;

    Name name;
    etl::string_view value;
};

using Fields = etl::vector<Field, 8>;

}
