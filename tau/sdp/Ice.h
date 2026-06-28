#pragma once

#include <etl/string.h>
#include <etl/string_view.h>
#include <etl/vector.h>
#include <initializer_list>

namespace tau::sdp {

using Candidate = etl::string_view;
using Candidates = etl::vector<Candidate, 8>;

struct Ice {
    using Ufrag = etl::string<32>;
    using Pwd = etl::string<32>;

    bool trickle = false; //TODO: types?
    Ufrag ufrag = {};
    Pwd pwd = {};
    Candidates candidates = {};
};

inline Candidates MakeCandidates(std::initializer_list<Candidate> list) {
    Candidates candidates;
    for(auto&& candidate : list) {
        if(candidates.full()) {
            break;
        }
        candidates.push_back(std::move(candidate));
    }
    return candidates;
}

}
