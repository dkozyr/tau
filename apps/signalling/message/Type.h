#pragma once

#include <etl/string_stream.h>

namespace tau::signalling::message {

enum class Type {
    kInit,
    kSdp,
    kIceCandidates,
    kClose,
    kError
};

inline Type TypeFromString(const etl::string_view& str) {
    if(str == "init")           { return Type::kInit; }
    if(str == "sdp")            { return Type::kSdp; }
    if(str == "ice_candidates") { return Type::kIceCandidates; }
    if(str == "close")          { return Type::kClose; }
    return Type::kError;
}

inline etl::string_stream& operator<<(etl::string_stream& ss, const Type& type) {
    switch(type) {
        case Type::kInit:          ss << "init"; break;
        case Type::kSdp:           ss << "sdp"; break;
        case Type::kIceCandidates: ss << "ice_candidates"; break;
        case Type::kClose:         ss << "close"; break;
        case Type::kError:         ss << "error"; break;
    }
    return ss;
}

}
