#pragma once

#include <etl/vector.h>
#include <initializer_list>
#include <cstdint>

namespace tau::rtp::session {

using SnsVector = etl::vector<uint16_t, 1024>;
using SnsIVector = etl::ivector<uint16_t>;

inline SnsVector ToVector(std::initializer_list<uint16_t> sns) {
    return SnsVector{sns};
}

}
