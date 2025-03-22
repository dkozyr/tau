#pragma once

#include <string>
#include <string_view>

namespace tau::sdp::attribute {

// https://www.rfc-editor.org/rfc/rfc8839.html#name-candidate-attribute
class CandidateReader {
public:
    static std::string_view GetFoundation(const std::string_view& value);
    static uint16_t GetComponentId(const std::string_view& value);
    static std::string_view GetTransport(const std::string_view& value);
    static uint32_t GetPriority(const std::string_view& value);
    static std::string_view GetAddress(const std::string_view& value);
    static uint16_t GetPort(const std::string_view& value);
    static std::string_view GetType(const std::string_view& value);
    static std::string_view GetExtParameters(const std::string_view& value);

    static bool Validate(const std::string_view& value);
};

class CandidateWriter {
public:
    static std::string Write(std::string_view foundation, uint16_t component_id,
        std::string_view transport, uint32_t priority, std::string_view address,
        uint16_t port, std::string_view type, std::string_view ext_parameters);
};

}
