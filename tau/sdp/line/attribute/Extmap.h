#pragma once

#include "tau/sdp/Direction.h"
#include <etl/string.h>
#include <etl/string_view.h>
#include <etl/string_stream.h>

namespace tau::sdp::attribute {
    
class ExtmapReader {
public:
    static uint8_t GetId(const etl::string_view& value);
    static Direction GetDirection(const etl::string_view& value);
    static etl::string_view GetUri(const etl::string_view& value);

    static bool Validate(const etl::string_view& value);
};

class ExtmapWriter {
public:
    static etl::string_stream& Write(etl::string_stream& ss, uint8_t id, etl::string_view uri, Direction direction = Direction::kSendRecv);
};

}
