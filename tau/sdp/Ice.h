#pragma once

#include <string>
#include <vector>

namespace tau::sdp {

struct Ice {
    bool trickle = false; //TODO: types?
    std::string ufrag = {};
    std::string pwd = {};
    std::vector<std::string> candidates = {};
};

}
