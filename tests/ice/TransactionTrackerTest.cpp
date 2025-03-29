#include "tau/ice/TransactionTracker.h"
#include "tau/stun/Header.h"
#include "tests/lib/Common.h"

namespace tau::ice {

class TransactionTrackerTest : public ::testing::Test {
public:
    TransactionTrackerTest()
        : _tracker(_clock)
        , _message_view({.ptr = _message.data(), .size = _message.size()})
    {}

public:
    TestClock _clock;
    TransactionTracker _tracker;
    std::array<uint8_t, stun::kMessageHeaderSize> _message;
    BufferView _message_view;
};

TEST_F(TransactionTrackerTest, Basic) {
    const auto tp = _clock.Now();
    ASSERT_EQ(tp - kMin, _tracker.GetLastTimepoint(42));

    _tracker.SetTransactionId(_message_view, 42);
    auto hash = stun::HeaderReader::GetTransactionIdHash(ToConst(_message_view));
    _clock.Add(10 * kMs);

    ASSERT_TRUE(_tracker.HasTransaction(hash));
    ASSERT_FALSE(_tracker.HasTransaction(hash + 1));

    auto result = *_tracker.HasTransaction(hash);
    ASSERT_EQ(tp, result.tp);
    ASSERT_EQ(42, result.pair_id);
    ASSERT_EQ(1, _tracker.GetCount());

    _tracker.RemoveTransaction(hash);
    ASSERT_FALSE(_tracker.HasTransaction(hash));
    ASSERT_EQ(0, _tracker.GetCount());

    ASSERT_EQ(tp, _tracker.GetLastTimepoint(42));
}

TEST_F(TransactionTrackerTest, WithoutPairId) {
    const auto tp = _clock.Now();
    _tracker.SetTransactionId(_message_view, std::nullopt);
    auto hash = stun::HeaderReader::GetTransactionIdHash(ToConst(_message_view));
    _clock.Add(10 * kMs);

    ASSERT_TRUE(_tracker.HasTransaction(hash));
    ASSERT_FALSE(_tracker.HasTransaction(hash + 1));

    auto result = *_tracker.HasTransaction(hash);
    ASSERT_EQ(tp, result.tp);
    ASSERT_EQ(std::nullopt, result.pair_id);
    ASSERT_EQ(1, _tracker.GetCount());

    _tracker.RemoveTransaction(hash);
    ASSERT_FALSE(_tracker.HasTransaction(hash));
    ASSERT_EQ(0, _tracker.GetCount());
}

TEST_F(TransactionTrackerTest, RemoveOldHashs) {
    constexpr auto kMaxHistorySize = 10;
    std::vector<uint32_t> hashs;
    for(size_t i = 0; i < 100; ++i) {
        _tracker.SetTransactionId(_message_view, i);
        auto hash = stun::HeaderReader::GetTransactionIdHash(ToConst(_message_view));
        hashs.push_back(hash);
        ASSERT_TRUE(_tracker.HasTransaction(hash));
        auto result = *_tracker.HasTransaction(hash);
        ASSERT_EQ(_clock.Now(), result.tp);
        ASSERT_EQ(i, result.pair_id);

        ASSERT_GE(kMaxHistorySize, _tracker.GetCount());
        for(size_t j = 0; j < hashs.size(); ++j) {
            if(j + _tracker.GetCount() < hashs.size()) {
                ASSERT_FALSE(_tracker.HasTransaction(hashs[j]));
            } else {
                ASSERT_TRUE(_tracker.HasTransaction(hashs[j]));
                auto result = *_tracker.HasTransaction(hashs[j]);
                const auto& target_pair_id = j;
                ASSERT_EQ(target_pair_id, result.pair_id);
            }
        }
        _clock.Add(TransactionTracker::kTimeoutDefault / kMaxHistorySize);
    }
}

}
