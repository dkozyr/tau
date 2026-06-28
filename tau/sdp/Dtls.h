#pragma once

#include <optional>
#include <etl/string.h>
#include <etl/string_view.h>

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
    etl::string<96> fingerprint_sha256 = {}; // only sha-256 is used as supported by all browsers for now
};

inline etl::string_view ToString(Setup setup) {
    switch(setup) {
        case Setup::kActpass:  return "actpass";
        case Setup::kActive:   return "active";
        case Setup::kPassive:  return "passive";
        case Setup::kHoldconn: return "holdconn";
    }
    return "unknown";
}

}
