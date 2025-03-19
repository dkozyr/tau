#pragma once

#include "tau/sdp/Ice.h"
#include "tau/sdp/Dtls.h"
#include "tau/sdp/Media.h"
#include <vector>
#include <optional>

namespace tau::sdp {

struct Sdp {
    // std::optional<Ice> ice = std::nullopt;
    // std::optional<Dtls> dtls = std::nullopt;
    std::vector<Media> medias = {};
};

}
