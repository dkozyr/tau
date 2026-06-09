#pragma once

#include <etl/string.h>
#include <etl/string_view.h>
#include <etl/vector.h>
#include <initializer_list>

namespace tau::sdp {

using Candidate = etl::string_view;
using Candidates = etl::vector<Candidate, 32>;

struct Ice {
    bool trickle = false; //TODO: types?
    etl::string<32> ufrag = {};
    etl::string<32> pwd = {};
    Candidates candidates = {};
};

inline Candidates MakeCandidates(std::initializer_list<Candidate> list) {
    Candidates candidates;
    for (auto&& candidate : list) {
        candidates.push_back(std::move(candidate));
    }
    return candidates;
}

}
