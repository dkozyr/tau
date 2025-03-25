#include "tau/ice/TransactionIdTracker.h"
#include "tau/stun/Header.h"
#include "tests/lib/Common.h"

namespace tau::ice {

class TransactionIdTrackerTest : public ::testing::Test {
public:
    using Type = TransactionIdTracker::Type;

public:
    TransactionIdTrackerTest()
        : _tracker(_clock)
        , _message_view({.ptr = _message.data(), .size = _message.size()})
    {}

public:
    TestClock _clock;
    TransactionIdTracker _tracker;
    std::array<uint8_t, stun::kMessageHeaderSize> _message;
    BufferView _message_view;
};

TEST_F(TransactionIdTrackerTest, Basic) {
    auto tp = _clock.Now();
    _tracker.SetTransactionId(_message_view, Type::kStunServer);
    auto id = stun::HeaderReader::GetTransactionIdHash(ToConst(_message_view));
    _clock.Add(10 * kMs);

    ASSERT_TRUE(_tracker.HasTransactionId(id));
    ASSERT_FALSE(_tracker.HasTransactionId(id + 1));

    auto result = *_tracker.HasTransactionId(id);
    ASSERT_EQ(Type::kStunServer, result.type);
    ASSERT_EQ(tp, result.tp);
    ASSERT_EQ(1, _tracker.GetCount());

    _tracker.RemoveTransactionId(id);
    ASSERT_FALSE(_tracker.HasTransactionId(id));
    ASSERT_EQ(0, _tracker.GetCount());

    ASSERT_FALSE(_tracker.IsTimeout(Type::kStunServer));
    _clock.Add(TransactionIdTracker::kStunServerKeepAlivePeriod);
    ASSERT_TRUE(_tracker.IsTimeout(Type::kStunServer));
}

TEST_F(TransactionIdTrackerTest, RemoveOldHashs) {
    constexpr auto kMaxHistorySize = TransactionIdTracker::kConnectivityCheckTimeout / TransactionIdTracker::kConnectivityCheckPeriod;
    std::vector<uint32_t> ids;
    for(size_t i = 0; i < 100; ++i) {
        _tracker.SetTransactionId(_message_view, Type::kConnectivityCheck);
        auto id = stun::HeaderReader::GetTransactionIdHash(ToConst(_message_view));
        ids.push_back(id);
        ASSERT_TRUE(_tracker.HasTransactionId(id).has_value());
        auto result = *_tracker.HasTransactionId(id);
        ASSERT_EQ(Type::kConnectivityCheck, result.type);
        ASSERT_EQ(_clock.Now(), result.tp);

        ASSERT_GE(kMaxHistorySize, _tracker.GetCount());
        for(size_t j = 0; j < ids.size(); ++j) {
            if(j + _tracker.GetCount() < ids.size()) {
                ASSERT_FALSE(_tracker.HasTransactionId(ids[j]));
            } else {
                ASSERT_TRUE(_tracker.HasTransactionId(ids[j]));
            }
        }
        _clock.Add(TransactionIdTracker::kConnectivityCheckPeriod);
    }
}

}
