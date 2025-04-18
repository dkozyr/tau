#pragma once

#include <optional>
#include <string>

namespace tau::sdp {

enum Setup {
    kActpass,
    kActive,
    kPassive,
    kHoldconn
};

// https://www.rfc-editor.org/rfc/rfc4572.html#section-5
struct Dtls {
    std::optional<Setup> setup = {};
    std::string fingerprint_sha256 = {}; // only sha-256 is used as supported by all browsers for now
};

}
