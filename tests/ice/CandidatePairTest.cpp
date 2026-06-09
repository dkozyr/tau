#include "tau/ice/CandidatePair.h"
#include "tests/lib/Common.h"

namespace tau::ice {

TEST(CandidateTest, LessOperator) {
    const std::vector<CandidatePair::State> states = {
        CandidatePair::State::kFrozen,
        CandidatePair::State::kWaiting,
        CandidatePair::State::kInProgress,
        CandidatePair::State::kSucceeded,
        CandidatePair::State::kNominating,
        CandidatePair::State::kNominated,
        CandidatePair::State::kFailed,
    };

    auto less_operator = [](CandidatePair::State a, CandidatePair::State b, uint64_t priority1 = 0, uint64_t priority2 = 0) {
        CandidatePair v1;
        CandidatePair v2;
        v1.state = a;
        v2.state = b;
        v1.priority = priority1;
        v2.priority = priority2;
        return v1 < v2;
    };

    for(size_t i = 0; i < 6; ++i) {
        ASSERT_FALSE(less_operator(CandidatePair::State::kFailed, states[i]));
        ASSERT_TRUE(less_operator(states[i], CandidatePair::State::kFailed));
    }
    for(size_t i = 0; i < 7; ++i) {
        ASSERT_FALSE(less_operator(states[i], states[i], 1, 2));
        ASSERT_FALSE(less_operator(states[i], states[i], 1, 1));
        ASSERT_TRUE(less_operator(states[i], states[i], 2, 1));
    }
}

TEST(CandidateTest, PairPriority) {
    auto local = Priority(CandidateType::kHost, 2);
    auto remote = Priority(CandidateType::kServRefl, 1);

    auto controlling1 = PairPriority(Role::kControlling, local, remote);
    auto controlling2 = PairPriority(Role::kControlling, remote, local);
    ASSERT_LT(1ull << 32, controlling1);
    ASSERT_LT(1ull << 32, controlling2);
    ASSERT_GT(controlling1, controlling2);

    auto controlled1 = PairPriority(Role::kControlled, local, remote);
    auto controlled2 = PairPriority(Role::kControlled, remote, local);
    ASSERT_LT(1ull << 32, controlled1);
    ASSERT_LT(1ull << 32, controlled2);
    ASSERT_LT(controlled1, controlled2);

    ASSERT_GT(controlling1, controlled1);
    ASSERT_LT(controlling2, controlled2);
}

}
