#include "tau/sdp/line/Media.h"
#include "tau/common/String.h"

#include "tau/common/Log.h"

namespace tau::sdp {

MediaType MediaReader::GetType(const std::string_view& value) {
    const auto tokens = Split(value, " ");
    return GetMediaTypeByName(tokens[0]);
}

uint16_t MediaReader::GetPort(const std::string_view& value) {
    const auto tokens = Split(value, " ");
    return StringToUnsigned<uint16_t>(tokens[1]).value();
}

std::string_view MediaReader::GetProtocol(const std::string_view& value) {
    const auto tokens = Split(value, " ");
    return tokens[2];
}

std::vector<uint8_t> MediaReader::GetFmts(const std::string_view& value) {
    const auto tokens = Split(value, " ");
    std::vector<uint8_t> fmts(tokens.size() - 3);
    for(size_t i = 3; i < tokens.size(); ++i) {
        fmts[i - 3] = StringToUnsigned<uint8_t>(tokens[i]).value();
    }
    return fmts;
}

bool MediaReader::Validate(const std::string_view& value) {
    const auto tokens = Split(value, " ");
    if(tokens.size() < 4) {
        return false;
    }
    if(MediaType::kUnknown == GetMediaTypeByName(tokens[0])) {
        return false;
    }
    if(!StringToUnsigned<uint16_t>(tokens[1])) {
        return false; // format <port>/<number of ports> isn't supported
    }

    for(size_t i = 3; i < tokens.size(); ++i) {
        auto fmt = StringToUnsigned<uint8_t>(tokens[i]);
        if(!fmt || (*fmt > 127)) {
            return false;
        }
    }
    return true;
}

std::string MediaWriter::Write(MediaType type, uint16_t port, std::string_view protocol, const std::vector<uint8_t>& fmts) {
    std::stringstream ss;
    switch(type) {
        case MediaType::kAudio: ss << "audio"; break;
        case MediaType::kVideo: ss << "video"; break;
        case MediaType::kText: ss << "text"; break;
        case MediaType::kApplication: ss << "application"; break;
        case MediaType::kMessage: ss << "message"; break;
        case MediaType::kUnknown: ss << "-"; break;
    }
    ss << " " << port << " " << protocol;
    for(auto pt : fmts) {
        ss << " " << (size_t)pt;
    }
    return ss.str();
}

}
