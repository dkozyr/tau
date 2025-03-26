#include "tau/ice/TransactionIdTracker.h"
#include "tau/stun/Header.h"
#include "tests/lib/Common.h"

namespace tau::ice {

class TransactionIdTrackerTest : public ::testing::Test {
public:
    static inline asio_udp::endpoint kEndpoint1{asio_ip::address_v4::from_string("1.2.3.4"), 55555};
    static inline asio_udp::endpoint kEndpoint2{asio_ip::address_v4::from_string("11.22.33.44"), 54321};

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
    const auto tp = _clock.Now();
    ASSERT_EQ(tp - kMin, _tracker.GetLastTimepoint(kEndpoint1));

    _tracker.SetTransactionId(_message_view, kEndpoint1);
    auto id = stun::HeaderReader::GetTransactionIdHash(ToConst(_message_view));
    _clock.Add(10 * kMs);

    ASSERT_TRUE(_tracker.HasTransactionId(id));
    ASSERT_FALSE(_tracker.HasTransactionId(id + 1));

    auto result = *_tracker.HasTransactionId(id);
    ASSERT_EQ(kEndpoint1, result.remote);
    ASSERT_EQ(tp, result.tp);
    ASSERT_EQ(1, _tracker.GetCount());

    _tracker.RemoveTransactionId(id);
    ASSERT_FALSE(_tracker.HasTransactionId(id));
    ASSERT_EQ(0, _tracker.GetCount());

    ASSERT_EQ(tp, _tracker.GetLastTimepoint(kEndpoint1));
}

TEST_F(TransactionIdTrackerTest, RemoveOldHashs) {
    constexpr auto kMaxHistorySize = 10;
    std::vector<uint32_t> ids;
    for(size_t i = 0; i < 100; ++i) {
        const auto& remote_endpoint = (i % 2 == 0) ? kEndpoint1 : kEndpoint2;
        _tracker.SetTransactionId(_message_view, remote_endpoint);
        auto id = stun::HeaderReader::GetTransactionIdHash(ToConst(_message_view));
        ids.push_back(id);
        ASSERT_TRUE(_tracker.HasTransactionId(id));
        auto result = *_tracker.HasTransactionId(id);
        ASSERT_EQ(remote_endpoint, result.remote);
        ASSERT_EQ(_clock.Now(), result.tp);

        ASSERT_GE(kMaxHistorySize, _tracker.GetCount());
        for(size_t j = 0; j < ids.size(); ++j) {
            if(j + _tracker.GetCount() < ids.size()) {
                ASSERT_FALSE(_tracker.HasTransactionId(ids[j]));
            } else {
                ASSERT_TRUE(_tracker.HasTransactionId(ids[j]));
                auto result = *_tracker.HasTransactionId(ids[j]);
                const auto& target_endpoint = (j % 2 == 0) ? kEndpoint1 : kEndpoint2;
                ASSERT_EQ(target_endpoint, result.remote);
            }
        }
        _clock.Add(TransactionIdTracker::kTimeoutDefault / kMaxHistorySize);
    }
}

}
