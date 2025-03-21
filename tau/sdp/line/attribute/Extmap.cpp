#include "tau/sdp/line/attribute/Extmap.h"
#include "tau/common/String.h"

namespace tau::sdp::attribute {

uint8_t ExtmapReader::GetId(const std::string_view& value) {
    const auto tokens = Split(value, " ");
    const auto& id_direction = tokens[0];
    const auto pos = id_direction.find('/');
    if(pos != std::string::npos) {
        return StringToUnsigned<uint8_t>(id_direction.substr(0, pos)).value();
    } else {
        return StringToUnsigned<uint8_t>(id_direction).value();
    }
}

Direction ExtmapReader::GetDirection(const std::string_view& value) {
    const auto tokens = Split(value, " ");
    const auto& id_direction = tokens[0];
    const auto pos = id_direction.find('/');
    if(pos != std::string::npos) {
        const auto direction = id_direction.substr(pos + 1);
        if(direction == "inactive") { return Direction::kInactive; }
        if(direction == "sendonly") { return Direction::kSend; }
        if(direction == "recvonly") { return Direction::kRecv; }
        // if(direction == "sendrecv") { return Direction::kSendRecv; }
    }
    return Direction::kSendRecv;
}

std::string_view ExtmapReader::GetUri(const std::string_view& value) {
    const auto tokens = Split(value, " ");
    return tokens[1];
}

bool ExtmapReader::Validate(const std::string_view& value) {
    const auto tokens = Split(value, " ");
    if(tokens.size() < 2) {
        return false;
    }
    const auto& id_direction = tokens[0];
    const auto pos = id_direction.find('/');
    const auto id = StringToUnsigned<uint8_t>(id_direction.substr(0, pos));
    if(!id || (*id == 0) || (*id > 14)) { // two-byte extension isn't supported
        return false;
    }
    //TODO: should we check direction?
    return true;
}

std::string ExtmapWriter::Write(uint8_t id, std::string_view uri, Direction direction) {
    std::stringstream ss;
    ss << (size_t)id;
    switch(direction) {
        case Direction::kInactive: ss << "/inactive"; break; //TODO: check it
        case Direction::kSend:     ss << "/sendonly"; break;
        case Direction::kRecv:     ss << "/recvonly"; break;
        case Direction::kSendRecv: break;
    }
    ss << " " << uri;
    return ss.str();
}

}
