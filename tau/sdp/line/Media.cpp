#include "tau/sdp/line/Media.h"
#include "tau/common/String.h"
#include <etl/string_stream.h>

namespace tau::sdp {

MediaType MediaReader::GetType(const etl::string_view& value) {
    const auto tokens = Split(value, " ");
    return GetMediaTypeByName(tokens[0]);
}

uint16_t MediaReader::GetPort(const etl::string_view& value) {
    const auto tokens = Split(value, " ");
    return StringToUnsigned<uint16_t>(tokens[1]).value();
}

etl::string_view MediaReader::GetProtocol(const etl::string_view& value) {
    const auto tokens = Split(value, " ");
    return tokens[2];
}

etl::vector<uint8_t, 32> MediaReader::GetFmts(const etl::string_view& value) {
    const auto tokens = Split(value, " ");
    etl::vector<uint8_t, 32> fmts(tokens.size() - 3);
    for(size_t i = 3; i < tokens.size(); ++i) {
        fmts[i - 3] = StringToUnsigned<uint8_t>(tokens[i]).value();
    }
    return fmts;
}

bool MediaReader::Validate(const etl::string_view& value) {
    const auto tokens = Split(value, " ");
    if(tokens.size() < 4) {
        return false;
    }
    const auto media_type = GetMediaTypeByName(tokens[0]);
    if(MediaType::kUnknown == media_type) {
        return false;
    }
    if(!StringToUnsigned<uint16_t>(tokens[1])) {
        return false; // format <port>/<number of ports> isn't supported
    }

    if(MediaType::kApplication == media_type) {
        return tokens[3] == "webrtc-datachannel";
    }
    for(size_t i = 3; i < tokens.size(); ++i) {
        auto fmt = StringToUnsigned<uint8_t>(tokens[i]);
        if(!fmt || (*fmt > 127)) {
            return false;
        }
    }
    return true;
}

etl::string_stream& MediaWriter::Write(etl::string_stream& ss, MediaType type, uint16_t port, etl::string_view protocol, const etl::vector<uint8_t, 32>& fmts) {
    switch(type) {
        case MediaType::kAudio:       ss << "audio"; break;
        case MediaType::kVideo:       ss << "video"; break;
        case MediaType::kText:        ss << "text"; break;
        case MediaType::kMessage:     ss << "message"; break;
        case MediaType::kUnknown:     ss << "-"; break;
        case MediaType::kApplication:
            ss << "application 9 UDP/DTLS/SCTP webrtc-datachannel";
            return ss;
    }
    ss << " " << port << " " << protocol;
    for(auto pt : fmts) {
        ss << " " << (size_t)pt;
    }
    return ss;
}

}
