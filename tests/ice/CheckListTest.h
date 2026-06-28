#pragma once

#include "CheckListTestParams.h"
#include "tau/ice/CheckList.h"
#include "tau/ice/StunClient.h"
#include "tau/stun/Writer.h"
#include "tau/stun/attribute/XorMappedAddress.h"
#include "tau/crypto/Random.h"
#include "tests/lib/Common.h"

namespace tau::ice {

using namespace tau::net;

class CheckListTest : public ::testing::TestWithParam<CheckListTestParams> {
public:
    static inline Endpoint kHostEndpoint1a{MakeIpAddressV4("1.2.3.4"), 55555};
    static inline Endpoint kHostEndpoint1b{MakeIpAddressV4("1.2.3.5"), 55000};
    static inline Endpoint kHostEndpoint1c{MakeIpAddressV4("1.2.3.6"), 55000};
    static inline Endpoint kHostEndpoint2a{MakeIpAddressV4("192.168.0.1"), 54321};
    static inline Endpoint kHostEndpoint2b{MakeIpAddressV4("192.168.0.2"), 54000};
    static inline Endpoint kHostEndpoint3a{MakeIpAddressV4("192.168.0.100"), 33300};
    static inline Endpoint kHostEndpoint3b{MakeIpAddressV4("192.168.0.101"), 44400};
    static inline Endpoint kServerReflexiveEndpoint1{MakeIpAddressV4("44.44.44.44"), 44444};
    static inline Endpoint kServerReflexiveEndpoint2{MakeIpAddressV4("55.55.55.55"), 55555};
    static inline Endpoint kServerReflexiveEndpoint3{MakeIpAddressV4("55.55.66.66"), 55777};
    static inline Endpoint kStunServerEndpoint{MakeIpAddressV4("88.77.66.55"), 43210};

public:
    CheckListTest() {
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
        _sockets1.push_back(kHostEndpoint1c);

        _sockets2.push_back(kHostEndpoint2a);
        _sockets2.push_back(kHostEndpoint2b);
    }

    void Init(const CheckListTestParams& params) {
        _nat1.emplace(_clock, NatEmulator::Options{
            .type = params.peer1_nat_type,
            .public_ip = kServerReflexiveEndpoint1.address});

        _nat2.emplace(_clock, NatEmulator::Options{
            .type = params.peer2_nat_type,
            .public_ip = kServerReflexiveEndpoint2.address});

        _sockets1.resize(params.peer1_sockets_count);
        _sockets2.resize(params.peer2_sockets_count);

        _check_list1.emplace(
            CheckList::Dependencies{.clock = _clock, .udp_allocator = g_udp_allocator},
            CheckList::Options{
                .role = Role::kControlling,
                .credentials = Credentials{
                    .local = _credentials.local,
                    .remote = _credentials.remote,
                },
                .sockets = _sockets1,
                .log_ctx = "[offer] "
            });

        _check_list2.emplace(
            CheckList::Dependencies{.clock = _clock, .udp_allocator = g_udp_allocator},
            CheckList::Options{
                .role = Role::kControlled,
                .credentials = Credentials{
                    .local = _credentials.remote,
                    .remote = _credentials.local,
                },
                .sockets = _sockets2,
                .log_ctx = "[answer] "
            });

        for(size_t i = 0; i < _sockets1.size(); ++i) {
            _stun_clients1.emplace_back(
                StunClient::Dependencies{_clock, g_udp_allocator}, kStunServerEndpoint
            );
            auto& stun_client = _stun_clients1.back();
            stun_client.SetCandidateCallback([this, idx = i](Endpoint reflexive) {
                _check_list1->AddLocalCandidate(CandidateType::kServRefl, idx, reflexive);
            });
            stun_client.SetSendCallback([this, socket = _sockets1[i]](Endpoint remote, Buffer&& message) {
                _nat1->Send(std::move(message), socket, remote);
            });
        }

        for(size_t i = 0; i < _sockets2.size(); ++i) {
            _stun_clients2.emplace_back(
                StunClient::Dependencies{_clock, g_udp_allocator}, kStunServerEndpoint
            );
            auto& stun_client = _stun_clients2.back();
            stun_client.SetCandidateCallback([this, idx = i](Endpoint reflexive) {
                _check_list2->AddLocalCandidate(CandidateType::kServRefl, idx, reflexive);
            });
            stun_client.SetSendCallback([this, socket = _sockets2[i]](Endpoint remote, Buffer&& message) {
                _nat2->Send(std::move(message), socket, remote);
            });
        }

        InitCallbacks();
    }

    void InitCallbacks() {
        _nat1->SetOnSendCallback([this](Buffer&& packet, Endpoint src, Endpoint dest) {
            if(dest == kStunServerEndpoint) {
                OnStunServerRequest(packet, src);
                _nat1->Recv(std::move(packet), dest, src);
            } else {
                _nat2->Recv(std::move(packet), src, dest);
            }
        });
        _nat1->SetOnRecvCallback([this](Buffer&& packet, Endpoint src, Endpoint dest) {
            for(size_t i = 0; i < _sockets1.size(); ++i) {
                if(_sockets1[i] == dest) {
                    if(_stun_clients1[i].IsServerEndpoint(src)) {
                        _stun_clients1[i].Recv(std::move(packet));
                    } else {
                        _check_list1->Recv(i, src, std::move(packet));
                    }
                    break;
                }
            }
        });

        _nat2->SetOnSendCallback([this](Buffer&& packet, Endpoint src, Endpoint dest) {
            if(dest == kStunServerEndpoint) {
                OnStunServerRequest(packet, src);
                _nat2->Recv(std::move(packet), dest, src);
            } else {
                _nat1->Recv(std::move(packet), src, dest);
            }
        });
        _nat2->SetOnRecvCallback([this](Buffer&& packet, Endpoint src, Endpoint dest) {
            for(size_t i = 0; i < _sockets2.size(); ++i) {
                if(_sockets2[i] == dest) {
                    if(_stun_clients2[i].IsServerEndpoint(src)) {
                        _stun_clients2[i].Recv(std::move(packet));
                    } else {
                        _check_list2->Recv(i, src, std::move(packet));
                    }
                    break;
                }
            }
        });

        _check_list1->SetSendCallback([this](size_t socket_idx, Endpoint remote, Buffer&& message) {
            _nat1->Send(std::move(message), _sockets1[socket_idx], remote);
        });
        _check_list2->SetSendCallback([this](size_t socket_idx, Endpoint remote, Buffer&& message) {
            _nat2->Send(std::move(message), _sockets2[socket_idx], remote);
        });

        _check_list1->SetCandidateCallback([this](CandidateStr candidate) {
            _check_list2->RecvRemoteCandidate(std::move(candidate));
        });
        _check_list2->SetCandidateCallback([this](CandidateStr candidate) {
            _check_list1->RecvRemoteCandidate(std::move(candidate));
        });
    }

    void AssertState(bool success) const {
        if(success) {
            ASSERT_NO_FATAL_FAILURE(AssertState(State::kCompleted));
        } else {
            ASSERT_NO_FATAL_FAILURE(AssertState(State::kFailed));
        }
    }

    void AssertState(State target) const {
        ASSERT_EQ(target, _check_list1->GetState());
        ASSERT_EQ(target, _check_list2->GetState());
    }

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
    std::optional<NatEmulator> _nat1;
    std::optional<NatEmulator> _nat2;

    etl::vector<Endpoint, 3> _sockets1;
    etl::vector<Endpoint, 3> _sockets2;

    etl::vector<StunClient, 3> _stun_clients1;
    etl::vector<StunClient, 3> _stun_clients2;

    std::optional<CheckList> _check_list1;
    std::optional<CheckList> _check_list2;

    etl::string<4>  _local_ufrag;
    etl::string<22> _local_password;
    etl::string<4>  _remote_ufrag;
    etl::string<22> _remote_password;
    Credentials _credentials;
};

TEST_P(CheckListTest, Main) {
    Init(GetParam());
    ASSERT_NO_FATAL_FAILURE(AssertState(State::kWaiting));

    _check_list1->Start();
    _check_list2->Start();
    ASSERT_NO_FATAL_FAILURE(AssertState(State::kRunning));

    for(size_t i = 0; i < 1000; ++i) {
        _clock.Add(42 * kMs);
        _check_list1->Process();
        _check_list2->Process();
        for(auto& stun_client : _stun_clients1) { stun_client.Process(); }
        for(auto& stun_client : _stun_clients2) { stun_client.Process(); }
        _nat1->Process();
        _nat2->Process();
    }

    ASSERT_NO_FATAL_FAILURE(AssertState(GetParam().success));
}

}
