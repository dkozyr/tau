#pragma once

#include <etl/string.h>
#include <etl/string_view.h>
#include <etl/string_stream.h>
#include <cstddef>

namespace tau::sdp {

// https://www.rfc-editor.org/rfc/rfc4566.html#section-5.8
class BandwidthReader {
public:
    static etl::string_view GetType(const etl::string_view& value);
    static size_t GetKbps(const etl::string_view& value);

    static bool Validate(const etl::string_view& value);
};

class BandwidthWriter {
public:
    static etl::string_stream& Write(etl::string_stream& ss, etl::string_view type, size_t kbps);
};

}
