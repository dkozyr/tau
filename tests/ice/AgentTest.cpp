#include "tests/ice/AgentTestParams.h"
#include "tau/ice/Agent.h"
#include "tau/stun/Writer.h"
#include "tau/stun/attribute/XorMappedAddress.h"
#include "tau/crypto/Random.h"
#include "tau/common/Event.h"
#include "tests/lib/Common.h"

namespace tau::ice {

// class AgentTest : public ::testing::TestWithParam<AgentTestParams> {
class AgentTest : public ::testing::Test {
public:
    static inline Endpoint kHostEndpoint1a{IpAddressV4::from_string("1.2.3.4"), 55555};
    static inline Endpoint kHostEndpoint1b{IpAddressV4::from_string("1.2.3.5"), 55000};
    static inline Endpoint kHostEndpoint2a{IpAddressV4::from_string("192.168.0.1"), 54321};
    static inline Endpoint kHostEndpoint2b{IpAddressV4::from_string("192.168.0.2"), 54000};
    static inline IpAddressV4 kServerReflexiveIp1{IpAddressV4::from_string("44.44.44.44")};
    static inline IpAddressV4 kServerReflexiveIp2{IpAddressV4::from_string("55.55.55.55")};
    static inline Endpoint kStunServerEndpoint{IpAddressV4::from_string("88.77.66.55"), 43210};
    static inline Endpoint kTurnServerEndpoint{IpAddressV4::from_string("99.99.99.99"), 3478};

public:
    AgentTest()
        : _credentials{
            .local = {.ufrag = crypto::RandomBase64(4), .password = crypto::RandomBase64(22)},
            .remote = {.ufrag = crypto::RandomBase64(4), .password = crypto::RandomBase64(22)},
        }
    {}

    void Init(const AgentTestParams& params) {
        _nat1.emplace(_clock, NatEmulator::Options{
            .type = params.peer1_nat_type,
            .public_ip = kServerReflexiveIp1});

        _nat2.emplace(_clock, NatEmulator::Options{
            .type = params.peer2_nat_type,
            .public_ip = kServerReflexiveIp1});

        _sockets1.resize(params.peer1_sockets_count);
        _sockets2.resize(params.peer2_sockets_count);

        _agent1.emplace(
            Agent::Dependencies{.clock = _clock, .udp_allocator = g_udp_allocator},
            Agent::Options{
                .role = Role::kControlling,
                .credentials = Credentials{
                    .local = _credentials.local,
                    .remote = _credentials.remote,
                },
                .interfaces = _sockets1,
                .stun_servers = {kStunServerEndpoint},
                .turn_servers = {},
                .log_ctx = "[offer] "
            });

        _agent2.emplace(
            Agent::Dependencies{.clock = _clock, .udp_allocator = g_udp_allocator},
            Agent::Options{
                .role = Role::kControlled,
                .credentials = Credentials{
                    .local = _credentials.remote,
                    .remote = _credentials.local,
                },
                .interfaces = _sockets1,
                .stun_servers = {kStunServerEndpoint},
                .turn_servers = {},
                .log_ctx = "[answer] "
            });

        InitCallbacks();
    }

    void InitCallbacks() {
        _nat1->SetOnSendCallback([this](Buffer&& packet, Endpoint src, Endpoint dest) {
            if(dest == kStunServerEndpoint) {
                OnStunServerRequest(packet, src);
                _nat1->Recv(std::move(packet), dest, src);
            } else if(dest == kTurnServerEndpoint) {
                //TODO: use test TURN server
            } else {
                _nat2->Recv(std::move(packet), src, dest);
            }
        });
        _nat1->SetOnRecvCallback([this](Buffer&& packet, Endpoint src, Endpoint dest) {
            for(size_t i = 0; i < _sockets1.size(); ++i) {
                if(_sockets1[i] == dest) {
                    _agent1->Recv(i, src, std::move(packet));
                    break;
                }
            }
        });

        _nat2->SetOnSendCallback([this](Buffer&& packet, Endpoint src, Endpoint dest) {
            if(dest == kStunServerEndpoint) {
                OnStunServerRequest(packet, src);
                _nat2->Recv(std::move(packet), dest, src);
            } else if(dest == kTurnServerEndpoint) {
                //TODO: use test TURN server
            } else {
                _nat1->Recv(std::move(packet), src, dest);
            }
        });
        _nat2->SetOnRecvCallback([this](Buffer&& packet, Endpoint src, Endpoint dest) {
            for(size_t i = 0; i < _sockets2.size(); ++i) {
                if(_sockets2[i] == dest) {
                    _agent2->Recv(i, src, std::move(packet));
                    break;
                }
            }
        });

        _agent1->SetSendCallback([this](size_t socket_idx, Endpoint remote, Buffer&& message) {
            _nat1->Send(std::move(message), _sockets1[socket_idx], remote);
        });
        _agent2->SetSendCallback([this](size_t socket_idx, Endpoint remote, Buffer&& message) {
            _nat2->Send(std::move(message), _sockets2[socket_idx], remote);
        });

        _agent1->SetCandidateCallback([this](std::string candidate) {
            _agent2->RecvRemoteCandidate(std::move(candidate));
        });
        _agent2->SetCandidateCallback([this](std::string candidate) {
            _agent1->RecvRemoteCandidate(std::move(candidate));
        });
    }

//     void AssertState(bool success) const {
//         if(success) {
//             ASSERT_NO_FATAL_FAILURE(AssertState(State::kCompleted));
//         } else {
//             ASSERT_NO_FATAL_FAILURE(AssertState(State::kFailed));
//         }
//     }

//     void AssertState(State target) const {
//         ASSERT_EQ(target, _check_list1->GetState());
//         ASSERT_EQ(target, _check_list2->GetState());
//     }

    //TODO: move to utils file?
    static void OnStunServerRequest(Buffer& message, Endpoint src) {
        stun::Writer writer(message.GetViewWithCapacity(), stun::kBindingResponse);
        stun::attribute::XorMappedAddressWriter::Write(writer,
            stun::AttributeType::kXorMappedAddress,
            src.address().to_v4().to_uint(),
            src.port());
        message.SetSize(writer.GetSize());
    }

protected:
    TestClock _clock;
    std::optional<NatEmulator> _nat1;
    std::optional<NatEmulator> _nat2;

    std::vector<Endpoint> _sockets1 = {kHostEndpoint1a, kHostEndpoint1b};
    std::vector<Endpoint> _sockets2 = {kHostEndpoint2a, kHostEndpoint2b};

    std::optional<Agent> _agent1;
    std::optional<Agent> _agent2;

    Credentials _credentials;
};

// TEST_P(AgentTest, Main) {
TEST_F(AgentTest, Main) {
    Init(AgentTestParams{
        .peer1_nat_type = NatEmulator::Type::kSymmetric,
        .peer1_sockets_count = 2,
        .peer1_has_turn = false,

        .peer2_nat_type = NatEmulator::Type::kPortRestrictedCone,
        .peer2_sockets_count = 2,
        .peer2_has_turn = false,
        
        .success = true
    });
//     Init(GetParam());
//     ASSERT_NO_FATAL_FAILURE(AssertState(State::kWaiting));

    _agent1->Start();
    _agent2->Start();
//     ASSERT_NO_FATAL_FAILURE(AssertState(State::kRunning));

    for(size_t i = 0; i < 1000; ++i) {
        _clock.Add(42 * kMs);
        _agent1->Process();
        _agent2->Process();
        _nat1->Process();
        _nat2->Process();
    }

//     ASSERT_NO_FATAL_FAILURE(AssertState(GetParam().success));
}

}
