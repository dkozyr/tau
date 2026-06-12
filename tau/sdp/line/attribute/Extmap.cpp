#include "tau/sdp/line/attribute/Extmap.h"
#include "tau/common/String.h"

namespace tau::sdp::attribute {

uint8_t ExtmapReader::GetId(const etl::string_view& value) {
    SplitTokens<1> tokens;
    Split(tokens, value, " ");
    const auto& id_direction = tokens[0];
    const auto pos = id_direction.find('/');
    if(pos != etl::string_view::npos) {
        return StringToUnsigned<uint8_t>(id_direction.substr(0, pos)).value();
    } else {
        return StringToUnsigned<uint8_t>(id_direction).value();
    }
}

Direction ExtmapReader::GetDirection(const etl::string_view& value) {
    SplitTokens<2> tokens;
    Split(tokens, value, " ");
    const auto& id_direction = tokens[0];
    const auto pos = id_direction.find('/');
    if(pos != etl::string_view::npos) {
        const auto direction = id_direction.substr(pos + 1);
        if(direction == "inactive") { return Direction::kInactive; }
        if(direction == "sendonly") { return Direction::kSend; }
        if(direction == "recvonly") { return Direction::kRecv; }
        // if(direction == "sendrecv") { return Direction::kSendRecv; }
    }
    return Direction::kSendRecv;
}

etl::string_view ExtmapReader::GetUri(const etl::string_view& value) {
    SplitTokens<2> tokens;
    Split(tokens, value, " ");
    return tokens[1];
}

bool ExtmapReader::Validate(const etl::string_view& value) {
    SplitTokens<2> tokens;
    Split(tokens, value, " ");
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

etl::string_stream& ExtmapWriter::Write(etl::string_stream& ss, uint8_t id, etl::string_view uri, Direction direction) {
    ss << (size_t)id;
    switch(direction) {
        case Direction::kInactive: ss << "/inactive"; break; //TODO: check it
        case Direction::kSend:     ss << "/sendonly"; break;
        case Direction::kRecv:     ss << "/recvonly"; break;
        case Direction::kSendRecv: break;
    }
    ss << " " << uri;
    return ss;
}

}
