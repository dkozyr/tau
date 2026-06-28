#pragma once

#include <etl/string.h>
#include <etl/string_view.h>
#include <etl/string_stream.h>
#include <cstdint>

namespace tau::sdp::attribute {

// https://www.rfc-editor.org/rfc/rfc8839.html#name-candidate-attribute
class CandidateReader {
public:
    static etl::string_view GetFoundation(const etl::string_view& value);
    static uint16_t GetComponentId(const etl::string_view& value);
    static etl::string_view GetTransport(const etl::string_view& value);
    static uint32_t GetPriority(const etl::string_view& value);
    static etl::string_view GetAddress(const etl::string_view& value);
    static uint16_t GetPort(const etl::string_view& value);
    static etl::string_view GetType(const etl::string_view& value);
    static etl::string_view GetExtParameters(const etl::string_view& value);

    static bool Validate(const etl::string_view& value);
};

class CandidateWriter {
public:
    static etl::string_stream& Write(etl::string_stream& ss,
        uint32_t foundation, uint16_t component_id,
        etl::string_view transport, uint32_t priority, etl::string_view address,
        uint16_t port, etl::string_view type, etl::string_view ext_parameters);
};

}
