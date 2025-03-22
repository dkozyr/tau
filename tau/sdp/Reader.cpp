#include "tau/sdp/Reader.h"
#include "tau/sdp/line/Media.h"
#include "tau/sdp/line/Originator.h"
#include "tau/sdp/line/Connection.h"
#include "tau/sdp/line/Bandwidth.h"
#include "tau/sdp/line/Attribute.h"
#include "tau/common/String.h"

namespace tau::sdp {

bool Reader::ForEachLine(const std::string_view& sdp, LineCallback callback) {
    std::optional<size_t> media_index;
    const auto end_of_line = (sdp.find('\r') != std::string::npos) ? "\r\n" : "\n"; //TODO: name constants
    const auto lines = Split(sdp, end_of_line);
    for(auto& line : lines) {
        if(line.empty()) { return true; }
        if((line.size() <= 2) || (line[1] != '=')) {
            return false;
        }
        auto type = ParseType(line[0]);
        if(!type) {
            continue;
        }
        if(type == kMedia) {
            media_index = media_index ? *media_index + 1 : 0;
        }
        if(!callback(media_index, *type, line.substr(2))) {
            return false;
        }
    }
    return true;
}

bool Reader::Validate(const std::string_view& sdp) {
    return ForEachLine(sdp, [](std::optional<size_t>, LineType type, std::string_view value) {
        switch(type) {
            case kOriginator: return OriginatorReader::Validate(value);
            case kMedia:      return MediaReader::Validate(value);
            case kConnection: return ConnectionReader::Validate(value);
            case kBandwidth:  return BandwidthReader::Validate(value);
            case kAttribute:  return AttributeReader::Validate(value);
            default:
                break;
        }
        return true;
    });
}

std::optional<LineType> Reader::ParseType(char c) {
    for(auto type : {kVersion, kOriginator, kSessionName, kMedia, kAttribute, kConnection,
                        kInformation, kBandwidth, kUri, kEmail, kPhone}) {
        if(c == type) {
            return type;
        }
    }
    return std::nullopt;
}

}
