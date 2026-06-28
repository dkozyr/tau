#pragma once

#include <etl/string.h>
#include <etl/string_view.h>
#include <etl/string_stream.h>
#include <cstdint>

namespace tau::sdp::attribute {
    
class RtcpFbReader {
public:
    static uint8_t GetPt(const etl::string_view& value);
    static etl::string_view GetValue(const etl::string_view& value);

    static bool Validate(const etl::string_view& value);
};

class RtcpFbWriter {
public:
    static etl::string_stream& Write(etl::string_stream& ss, uint8_t pt, etl::string_view value);
};

}
