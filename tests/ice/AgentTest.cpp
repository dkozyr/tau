#include "AgentTestParams.h"
#include "TurnServerEmulator.h"
#include "tau/ice/Agent.h"
#include "tau/stun/Writer.h"
#include "tau/stun/attribute/XorMappedAddress.h"
#include "tau/crypto/Random.h"
#include "tests/lib/Common.h"

namespace tau::ice {

using namespace tau::net;

class AgentTest : public ::testing::TestWithParam<AgentTestParams> {
public:
    static inline Endpoint kHostEndpoint1a{MakeIpAddressV4("1.2.3.4"), 55555};
    static inline Endpoint kHostEndpoint1b{MakeIpAddressV4("1.2.3.5"), 55000};
    static inline Endpoint kHostEndpoint2a{MakeIpAddressV4("192.168.0.1"), 54321};
    static inline Endpoint kHostEndpoint2b{MakeIpAddressV4("192.168.0.2"), 54000};
    static inline IpAddress kServerReflexiveIp1{MakeIpAddressV4("44.44.44.44")};
    static inline IpAddress kServerReflexiveIp2{MakeIpAddressV4("55.55.55.55")};
    static inline Endpoint kStunServerEndpoint{MakeIpAddressV4("88.77.66.55"), 43210};
    static inline IpAddress kTurnServerIp1 = MakeIpAddressV4("222.222.222.222");
    static inline IpAddress kTurnServerIp2 = MakeIpAddressV4("210.210.210.210");

public:
    AgentTest()
        : _turn_server1(_clock, TurnServerEmulator::Options{.public_ip = kTurnServerIp1})
        , _turn_server2(_clock, TurnServerEmulator::Options{.public_ip = kTurnServerIp2})
    {
        crypto::RandomBase64(_local_ufrag, 4);
        crypto::RandomBase64(_local_password, 22);
        crypto::RandomBase64(_remote_ufrag, 4);
        crypto::RandomBase64(_remote_password, 22);

        _credentials = {
            .local = {.ufrag = _local_ufrag, .password = _local_password},
            .remote = {.ufrag = _remote_ufrag, .password = _remote_password},
        };

        _sockets1.push_back(kHostEndpoint1a);
        _sockets1.push_back(kHostEndpoint1b);

        _sockets2.push_back(kHostEndpoint2a);
        _sockets2.push_back(kHostEndpoint2b);
    }

    void Init(const AgentTestParams& params) {
        _nat1.emplace(_clock, NatEmulator::Options{
            .type = params.peer1_nat_type,
            .public_ip = kServerReflexiveIp1,
            .drop_rate = 0 //TODO: test it
        });

        _nat2.emplace(_clock, NatEmulator::Options{
            .type = params.peer2_nat_type,
            .public_ip = kServerReflexiveIp2,
            .drop_rate = 0 //TODO: test it
        });

        _sockets1.resize(params.peer1_sockets_count);
        _sockets2.resize(params.peer2_sockets_count);

        etl::vector<Endpoint, 2> stun_servers;
        stun_servers.push_back(kStunServerEndpoint);

        _agent1.emplace(
            Agent::Dependencies{.clock = _clock, .udp_allocator = g_udp_allocator},
            Agent::Options{
                .role = Role::kControlling,
                .credentials = Credentials{
                    .local = _credentials.local,
                    .remote = _credentials.remote,
                },
                .interfaces = _sockets1,
                .stun_servers = stun_servers,
                .turn_servers = CreateTurnServersOptions(params.peer1_has_turn),
                .nominating_strategy = params.nominating_strategy_best
                    ? Agent::NominatingStrategy::kBestValid
                    : Agent::NominatingStrategy::kFirstValid,
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
                .stun_servers = stun_servers,
                .turn_servers = CreateTurnServersOptions(params.peer2_has_turn),
                .log_ctx = "[answer] "
            });

        InitCallbacks();
    }

    void InitCallbacks() {
        _nat1->SetOnSendCallback([this](Buffer&& packet, Endpoint src, Endpoint dest) {
            if(dest == kStunServerEndpoint) {
                OnStunServerRequest(packet, src);
                _nat1->Recv(std::move(packet), dest, src);
            } else if(dest.address == kTurnServerIp1) {
                _turn_server1.Recv(std::move(packet), src, dest);
            } else if(dest.address == kTurnServerIp2) {
                _turn_server2.Recv(std::move(packet), src, dest);
            } else {
                _nat2->Recv(std::move(packet), src, dest);
            }
        });
        _nat1->SetOnRecvCallback([this](Buffer&& packet, Endpoint src, Endpoint dest) {
            for(size_t i = 0; i < _sockets1.size(); ++i) {
                if(_sockets1[i] == dest) {
                    _agent1->Recv(i, src, std::move(packet));
                    return;
                }
            }
        });

        _nat2->SetOnSendCallback([this](Buffer&& packet, Endpoint src, Endpoint dest) {
            if(dest == kStunServerEndpoint) {
                OnStunServerRequest(packet, src);
                _nat2->Recv(std::move(packet), dest, src);
            } else if(dest.address == kTurnServerIp1) {
                _turn_server1.Recv(std::move(packet), src, dest);
            } else if(dest.address == kTurnServerIp2) {
                _turn_server2.Recv(std::move(packet), src, dest);
            } else {
                _nat1->Recv(std::move(packet), src, dest);
            }
        });
        _nat2->SetOnRecvCallback([this](Buffer&& packet, Endpoint src, Endpoint dest) {
            for(size_t i = 0; i < _sockets2.size(); ++i) {
                if(_sockets2[i] == dest) {
                    _agent2->Recv(i, src, std::move(packet));
                    return;
                }
            }
        });

        _turn_server1.SetOnSendCallback([this](Buffer&& message, Endpoint src, Endpoint dest) {
            if(dest.address == kServerReflexiveIp1) {
                _nat1->Recv(std::move(message), src, dest);
            } else if(dest.address == kServerReflexiveIp2) {
                _nat2->Recv(std::move(message), src, dest);
            } else if(dest.address == kTurnServerIp1) {
                _turn_server1.Recv(std::move(message), src, dest);
            } else if(dest.address == kTurnServerIp2) {
                _turn_server2.Recv(std::move(message), src, dest);
            } else {
                // TAU_LOG_WARNING("Unknown dest: " << ToString(dest) << ", src: " << ToString(src));
            }
        });

        _turn_server2.SetOnSendCallback([this](Buffer&& message, Endpoint src, Endpoint dest) {
            if(dest.address == kServerReflexiveIp1) {
                _nat1->Recv(std::move(message), src, dest);
            } else if(dest.address == kServerReflexiveIp2) {
                _nat2->Recv(std::move(message), src, dest);
            } else if(dest.address == kTurnServerIp1) {
                _turn_server1.Recv(std::move(message), src, dest);
            } else if(dest.address == kTurnServerIp2) {
                _turn_server2.Recv(std::move(message), src, dest);
            } else {
                // TAU_LOG_WARNING("Unknown dest: " << ToString(dest) << ", src: " << ToString(src));
            }
        });

        _agent1->SetSendCallback([this](size_t socket_idx, Endpoint remote, Buffer&& message) {
            _nat1->Send(std::move(message), _sockets1[socket_idx], remote);
        });
        _agent2->SetSendCallback([this](size_t socket_idx, Endpoint remote, Buffer&& message) {
            _nat2->Send(std::move(message), _sockets2[socket_idx], remote);
        });

        _agent1->SetCandidateCallback([this](CandidateStr candidate) {
            _agent2->RecvRemoteCandidate(std::move(candidate));
        });
        _agent2->SetCandidateCallback([this](CandidateStr candidate) {
            _agent1->RecvRemoteCandidate(std::move(candidate));
        });

        _agent1->SetStateCallback([this](State state) {
            _agent1_states.push_back(state);
        });
        _agent2->SetStateCallback([this](State state) {
            _agent2_states.push_back(state);
        });
    }

    void AssertState(bool success) const {
        etl::vector<State, 32> target_states;
        if(success) {
            target_states.push_back(State::kRunning);
            target_states.push_back(State::kReady);
            target_states.push_back(State::kCompleted);
        } else {
            target_states.push_back(State::kRunning);
            target_states.push_back(State::kFailed);
        }
        ASSERT_EQ(target_states, _agent1_states);
        ASSERT_EQ(target_states, _agent2_states);
    }

    etl::unordered_map<Endpoint, PeerCredentials, 3> CreateTurnServersOptions(bool enable) {
        etl::unordered_map<Endpoint, PeerCredentials, 3> turn_servers;
        if(enable) {
            _ufrag_local_storage.push_back(etl::string<6>{6, 0});
            crypto::RandomBase64(_ufrag_local_storage.back(), 6);

            turn_servers[Endpoint{kTurnServerIp1, 3478}] = PeerCredentials{
                .ufrag = _ufrag_local_storage.back(),
                .password = "password"
            };
        }
        return turn_servers;
    }

    //TODO: move to utils file?
    static void OnStunServerRequest(Buffer& message, Endpoint src) {
        stun::Writer writer(message.GetViewWithCapacity(), stun::kBindingResponse);
        stun::attribute::XorMappedAddressWriter::Write(writer,
            stun::AttributeType::kXorMappedAddress,
            src.address.GetUint32(),
            src.port);
        message.SetSize(writer.GetSize());
    }

protected:
    TestClock _clock;
    TurnServerEmulator _turn_server1;
    TurnServerEmulator _turn_server2;
    std::optional<NatEmulator> _nat1;
    std::optional<NatEmulator> _nat2;

    etl::vector<Endpoint, 3> _sockets1;
    etl::vector<Endpoint, 3> _sockets2;

    std::optional<Agent> _agent1;
    std::optional<Agent> _agent2;

    etl::vector<State, 32> _agent1_states;
    etl::vector<State, 32> _agent2_states;

    etl::string<4>  _local_ufrag;
    etl::string<22> _local_password;
    etl::string<4>  _remote_ufrag;
    etl::string<22> _remote_password;
    Credentials _credentials;

    etl::vector<etl::string<6>, 8> _ufrag_local_storage; //workaround for string_view
};

TEST_P(AgentTest, Main) {
    TAU_LOG_INFO("GetParam(): " << GetParam());
    Init(GetParam());

    _agent1->Start();
    _agent2->Start();

    for(size_t i = 0; i < 1'000; ++i) {
        _clock.Add(10 * kMs);
        _agent1->Process();
        _agent2->Process();
        _nat1->Process();
        _nat2->Process();
    }

    ASSERT_NO_FATAL_FAILURE(AssertState(GetParam().success));
}

std::vector<AgentTestParams> MakeAgentTestParams(bool use_turn) {
    const std::vector<NatEmulator::Type> nat_types = {
        NatEmulator::Type::kFullCone,
        NatEmulator::Type::kRestrictedCone,
        NatEmulator::Type::kPortRestrictedCone,
        NatEmulator::Type::kSymmetric,
        // NatEmulator::Type::kLocalNetworkOnly,
    };

    const size_t turn_parameters = use_turn ? 4 : 1;
 
    std::vector<AgentTestParams> params;
    for(size_t i = 0; i < nat_types.size(); ++i) {
    for(size_t j = i; j < nat_types.size(); ++j) {
        bool success = true;
        if(!use_turn) {
            if(nat_types[i] == NatEmulator::Type::kSymmetric) {
                success = false;
            }
            if(nat_types[i] == NatEmulator::Type::kPortRestrictedCone) {
                if(nat_types[j] == NatEmulator::Type::kSymmetric) {
                    success = false;
                }
            }
        }

        auto s1 = g_random.Int<size_t>(1, 2);
        auto s2 = g_random.Int<size_t>(1, 2);
        for(size_t t = use_turn; t < turn_parameters; ++t) {
            auto peer1_use_turn = ((t & 1) == 1);
            auto peer2_use_turn = ((t & 2) == 2);
            auto nominating_strategy_best = g_random.Bool();
            params.push_back(AgentTestParams{
                .peer1_nat_type = nat_types[i],
                .peer1_sockets_count = s1,
                .peer1_has_turn = peer1_use_turn,
                .peer2_nat_type = nat_types[j],
                .peer2_sockets_count = s2,
                .peer2_has_turn = peer2_use_turn,
                .nominating_strategy_best = nominating_strategy_best,
                .success = success
            });
            params.push_back(AgentTestParams{
                .peer1_nat_type = nat_types[j],
                .peer1_sockets_count = s1,
                .peer1_has_turn = peer1_use_turn,
                .peer2_nat_type = nat_types[i],
                .peer2_sockets_count = s2,
                .peer2_has_turn = peer2_use_turn,
                .nominating_strategy_best = !nominating_strategy_best,
                .success = success
            });
        }
    }}
    return params;
}

INSTANTIATE_TEST_SUITE_P(WithoutTurn, AgentTest, ::testing::ValuesIn(MakeAgentTestParams(false)));
INSTANTIATE_TEST_SUITE_P(WithTurn,    AgentTest, ::testing::ValuesIn(MakeAgentTestParams(true)));

}
