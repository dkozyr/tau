#pragma once

#include <etl/string_stream.h>
#include <cstdint>

namespace tau::rtp::session {

enum Event {
    kFir,
    kPli
};

inline etl::string_stream& ToString(etl::string_stream& ss, const Event& x) {
    switch(x) {
        case Event::kFir: return ss << "fir";
        case Event::kPli: return ss << "pli";
    }
    return ss << "unknown";
}

}
