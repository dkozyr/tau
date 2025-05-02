#pragma once

#include <cstdint>
#include <ostream>

namespace tau::rtp::session {

enum Event {
    kFir,
    kPli
};

inline std::ostream& operator<<(std::ostream& s, const Event& x) {
    switch(x) {
        case Event::kFir: return s << "fir";
        case Event::kPli: return s << "pli";
    }
    return s << "unknown";
}

}
