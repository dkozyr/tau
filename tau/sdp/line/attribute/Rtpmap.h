#pragma once

#include <etl/string.h>
#include <etl/string_view.h>
#include <etl/string_stream.h>
#include <cstdint>

namespace tau::sdp::attribute {
    
class RtpmapReader {
public:
    static uint8_t GetPt(const etl::string_view& value);
    static etl::string_view GetEncodingName(const etl::string_view& value);
    static size_t GetClockRate(const etl::string_view& value);
    static etl::string_view GetParams(const etl::string_view& value);

    static bool Validate(const etl::string_view& value);
};

class RtpmapWriter {
public:
    static etl::string_stream& Write(etl::string_stream& ss, uint8_t pt, etl::string_view encoding_name, size_t clock_rate, etl::string_view params = {});
};

}
