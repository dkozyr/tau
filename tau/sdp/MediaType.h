#pragma once

#include <string_view>

namespace tau::sdp {

enum MediaType {
    kUnknown     = 0,
    kAudio       = 1,
    kVideo       = 2,
    kText        = 3,
    kApplication = 4,
    kMessage     = 5,
};

inline MediaType GetMediaTypeByName(std::string_view name) {
    if(name == "audio")       { return MediaType::kAudio; }
    if(name == "video")       { return MediaType::kVideo; }
    if(name == "text")        { return MediaType::kText; }
    if(name == "application") { return MediaType::kApplication; }
    if(name == "message")     { return MediaType::kMessage; }
    return MediaType::kUnknown;
}

}
