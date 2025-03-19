#pragma once

#include <string>

namespace tau::sdp {

// https://www.rfc-editor.org/rfc/rfc4572.html#section-5
struct Dtls {
    std::string hash_func = {};
    std::string fingerprint = {};
};

}
