#include "tau/ice/StunClient.h"
#include "tau/ice/Constants.h"
#include "tau/stun/Writer.h"
#include "tau/stun/attribute/XorMappedAddress.h"
#include "tau/stun/attribute/Fingerprint.h"
#include "tests/ice/NatEmulator.h"
#include "tests/lib/Common.h"

namespace tau::ice {

class StunClientTest : public ::testing::Test {
public:
    static inline Endpoint kServerEndpoint{asio_ip::address_v4::from_string("77.77.77.77"), 33333};
    static inline Endpoint kClientEndpoint{asio_ip::address_v4::from_string("192.168.0.77"), 44444};

public:
    StunClientTest() {
        _nat.emplace(_clock, NatEmulator::Options{.type = NatEmulator::Type::kSymmetric});
    }

    void Init() {
        _client.emplace(
            StunClient::Dependencies{
                .clock = _clock,
                .udp_allocator = g_udp_allocator
            },
            kServerEndpoint
        );
        InitCallbacks();
    }

    void InitCallbacks() {
        _client->SetCandidateCallback([this](Endpoint reflexive) {
            _local_candidates.push_back(reflexive);
        });
        _client->SetSendCallback([this](Endpoint remote, Buffer&& message) {
            _nat->Send(std::move(message), kClientEndpoint, remote);
            _send_packets_count++;
        });
        _nat->SetOnSendCallback([this](Buffer&& message, Endpoint src, Endpoint dest) {
            if(dest == kServerEndpoint) {
                OnStunServerRequest(message, src);
                _client->Recv(std::move(message));
            }
        });
    }

protected:
    //TODO: move to helper functions file?
    static void OnStunServerRequest(Buffer& message, Endpoint src) {
        stun::Writer writer(message.GetViewWithCapacity(), stun::kBindingResponse);
        stun::attribute::XorMappedAddressWriter::Write(writer,
            stun::AttributeType::kXorMappedAddress,
            src.address().to_v4().to_uint(),
            src.port());
        stun::attribute::FingerprintWriter::Write(writer);
        message.SetSize(writer.GetSize());
    }

protected:
    TestClock _clock;

    std::optional<StunClient> _client;
    std::optional<NatEmulator> _nat;

    std::vector<Endpoint> _local_candidates;
    size_t _send_packets_count = 0;
};

TEST_F(StunClientTest, IsServerEndpoint) {
    Init();
    ASSERT_TRUE(_client->IsServerEndpoint(kServerEndpoint));
    ASSERT_FALSE(_client->IsServerEndpoint(kClientEndpoint));
}

TEST_F(StunClientTest, Stun) {
    Init();

    _client->Process();
    _clock.Add(50 * kMs);
    _nat->Process();

    ASSERT_EQ(1, _send_packets_count);
    ASSERT_EQ(1, _local_candidates.size());
    const auto& reflexive = _local_candidates[0];
    ASSERT_EQ(NatEmulator::kPublicIpDefault, reflexive.address());

    for(size_t i = 2; i < 10; ++i) {
        _clock.Add(kStunServerKeepAlivePeriod / 2);
        _client->Process();
        ASSERT_EQ(i - 1, _send_packets_count);

        _clock.Add(kStunServerKeepAlivePeriod / 2);
        _client->Process();
        ASSERT_EQ(i, _send_packets_count);

        _clock.Add(50 * kMs);
        _nat->Process();
        ASSERT_EQ(1, _local_candidates.size());
    }
}

TEST_F(StunClientTest, StunRequestRetransmitOnLost) {
    _nat.emplace(_clock, NatEmulator::Options{.type = NatEmulator::Type::kLocalNetworkOnly});
    Init();

    for(size_t i = 1; i < 10; ++i) { 
        _clock.Add(kRtoDefault);
        _client->Process();
        ASSERT_EQ(i, _send_packets_count);

        _clock.Add(50 * kMs);
        _nat->Process();
        ASSERT_EQ(0, _local_candidates.size());
    }
}

}
