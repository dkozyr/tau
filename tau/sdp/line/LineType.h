#pragma once

namespace tau::sdp {

// https://www.rfc-editor.org/rfc/rfc4566.html#section-5
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
