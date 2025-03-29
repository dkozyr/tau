#include "tau/ice/Candidate.h"
#include "tests/lib/Common.h"
#include <unordered_set>

namespace tau::ice {

TEST(CandidateTest, Priority) {
    uint32_t prev_priority = std::numeric_limits<uint32_t>::max();
    for(auto type : {CandidateType::kHost, CandidateType::kPeerRefl, CandidateType::kServRefl, CandidateType::kRelayed}) {
        for(size_t socket_idx = 0; socket_idx < 100; ++socket_idx) {
            auto priority = Priority(type, socket_idx);
            ASSERT_LT(priority, prev_priority);
            prev_priority = priority;
        }
    }
}

TEST(CandidateTest, Foundation) {
    std::unordered_set<uint32_t> foundations;
    for(auto type : {CandidateType::kHost, CandidateType::kPeerRefl, CandidateType::kServRefl, CandidateType::kRelayed}) {
        for(size_t socket_idx = 0; socket_idx < 100; ++socket_idx) {
            auto foundation = Priority(type, socket_idx);
            ASSERT_FALSE(Contains(foundations, foundation));
            foundations.insert(foundation);
        }
    }
}

TEST(CandidateTest, Type) {
    for(auto type : {CandidateType::kHost, CandidateType::kPeerRefl, CandidateType::kServRefl, CandidateType::kRelayed}) {
        auto type_str = CandidateTypeToString(type);
        auto type_parsed = CandidateTypeFromString(type_str);
        ASSERT_EQ(type, type_parsed);
    }
}

TEST(CandidateTest, MalformedTypeParsedAsRelayed) {
    auto type = CandidateTypeFromString("malformed");
    ASSERT_EQ(CandidateType::kRelayed, type);
}

}
