#pragma once

#include "tau/common/String.h"

namespace tau::sdp {

// https://datatracker.ietf.org/doc/html/rfc4566#section-5
enum LineType : char {
    kVersion     = 'v',
    kOriginator  = 'o',
    kSessionName = 's',
    kMedia       = 'm',
    kAttribute   = 'a',
    kConnection  = 'c',
    kInformation = 'i',
    kBandwidth   = 'b',
    kUri         = 'u',
    kEmail       = 'e',
    kPhone       = 'p',
};

}
