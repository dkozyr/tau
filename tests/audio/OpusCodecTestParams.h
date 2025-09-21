#include "tests/lib/Common.h"

namespace tau::audio {

struct OpusCodecTestParams {
    uint32_t sample_rate;
    uint32_t channels;
};

inline std::ostream& operator<<(std::ostream& s, const OpusCodecTestParams& x) {
    return s << "sample_rate: " << x.sample_rate << ", channels: " << x.channels;
}

}
