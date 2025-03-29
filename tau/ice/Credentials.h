#pragma once

#include <string>

namespace tau::ice {

struct PeerCredentials {
    std::string ufrag;
    std::string password;
};

struct Credentials {
    PeerCredentials local;
    PeerCredentials remote;
};

}
