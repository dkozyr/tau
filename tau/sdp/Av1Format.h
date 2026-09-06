#pragma once

#include <tau/sdp/Media.h>

namespace tau::sdp {

struct Av1Format {
    uint8_t profile = 0;
    uint8_t level_index = 5;
    uint8_t tier = 0;
};

std::optional<Av1Format> ParseAv1Format(etl::string_view format);
Codec::Format CreateAv1Format(const Av1Format& format);

}
