#pragma once

#include "tau/sdp/MediaType.h"
#include <etl/string.h>
#include <etl/string_view.h>
#include <etl/string_stream.h>
#include <etl/vector.h>
#include <cstdint>

namespace tau::sdp {

// https://www.rfc-editor.org/rfc/rfc4566.html#section-5.14
class MediaReader {
public:
    static MediaType GetType(const etl::string_view& value);
    static uint16_t GetPort(const etl::string_view& value);
    static etl::string_view GetProtocol(const etl::string_view& value);
    static etl::vector<uint8_t, 32> GetFmts(const etl::string_view& value);

    static bool Validate(const etl::string_view& value);
};

class MediaWriter {
public:
    static etl::string_stream& Write(etl::string_stream& ss, MediaType type, uint16_t port, etl::string_view protocol, const etl::vector<uint8_t, 32>& fmts);
};

}
