#pragma once

#include <etl/string.h>
#include <etl/string_view.h>
#include <etl/string_stream.h>

namespace tau::sdp {

// https://www.rfc-editor.org/rfc/rfc4566.html#section-5.13
class AttributeReader {
public:
    static etl::string_view GetType(const etl::string_view& value);
    static etl::string_view GetValue(const etl::string_view& value);

    static bool Validate(const etl::string_view& value);
};

class AttributeWriter {
public:
    static etl::string_stream& Write(etl::string_stream& ss, etl::string_view type, etl::string_view value);
};

}
