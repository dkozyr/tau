#include "tau/dtls/Session.h"
#include "tests/lib/Common.h"

namespace tau::dtls {

class SessionTest : public ::testing::Test {
public:
    SessionTest() {
        Init();
    }

    void Init() {
        _client.emplace(
            Session::Dependencies{.udp_allocator = g_udp_allocator, .certificate = _client_certificate},
            Session::Options{_client_options});
        _server.emplace(
            Session::Dependencies{.udp_allocator = g_udp_allocator, .certificate = _server_certificate},
            Session::Options{_server_options});

        _client->SetSendCallback([&](Buffer&& packet) {
            _queue.push(std::make_pair(false, std::move(packet)));
        });
        _server->SetSendCallback([&](Buffer&& packet) {
            _queue.push(std::make_pair(true, std::move(packet)));
        });

        _client->SetRecvCallback([&](Buffer&& packet) {
            auto view = packet.GetView();
            auto message = etl::string_view{reinterpret_cast<char*>(view.ptr), view.size};
            TAU_LOG_INFO("[client] [recv] message: " << message);
            EXPECT_EQ(message, "Hello from server!");
        });
        _server->SetRecvCallback([&](Buffer&& packet) {
            auto view = packet.GetView();
            auto message = etl::string_view{reinterpret_cast<char*>(view.ptr), view.size};
            TAU_LOG_INFO("[server] [recv] message: " << message);
            EXPECT_EQ(message, "Hello from client!");
        });

        _client->SetStateCallback([&](Session::State state) { _client_states.push_back(state); });
        _server->SetStateCallback([&](Session::State state) { _server_states.push_back(state); });
    }

    void Process() {
        while(true) {
            _client->Process();
            _server->Process();

            if(_queue.empty()) {
                break;
            }

            auto& [from_server, packet] = _queue.front();
            if(from_server) {
                _client->Recv(std::move(packet));
            } else {
                _server->Recv(std::move(packet));
            }
            _queue.pop();
        }
    }

    void AssertSendData() {
        constexpr etl::string_view client_message = "Hello from client!";
        ASSERT_TRUE(_client->Send(
            BufferViewConst{.ptr = (const uint8_t*)client_message.data(), .size = client_message.size()}));
        
        constexpr etl::string_view server_message = "Hello from server!";
        ASSERT_TRUE(_server->Send(
            BufferViewConst{.ptr = (const uint8_t*)server_message.data(), .size = server_message.size()}));

        Process();
    }

    static void AssertStates(const etl::ivector<Session::State>& actual, std::initializer_list<Session::State> target) {
        ASSERT_EQ(actual.size(), target.size());
        size_t i = 0;
        for(auto target_state : target) {
            ASSERT_EQ(target_state, actual[i]);
            i++;
        }
    }

    void AssertSrtpProfile(Session::SrtpProfile target_profile) const {
        ASSERT_EQ(target_profile, _client->GetSrtpProfile().value());
        ASSERT_EQ(target_profile, _server->GetSrtpProfile().value());
    }

    void AssertKeyingMaterial() const {
        auto client_encrypting = _client->GetKeyingMaterial(true);
        auto client_decrypting = _client->GetKeyingMaterial(false);
        auto server_encrypting = _server->GetKeyingMaterial(true);
        auto server_decrypting = _server->GetKeyingMaterial(false);
        ASSERT_FALSE(client_encrypting.key.empty());
        ASSERT_FALSE(client_encrypting.salt.empty());
        ASSERT_FALSE(server_encrypting.key.empty());
        ASSERT_FALSE(server_encrypting.salt.empty());
        ASSERT_EQ(client_encrypting.key,  server_decrypting.key);
        ASSERT_EQ(client_encrypting.salt, server_decrypting.salt);
        ASSERT_EQ(client_decrypting.key,  server_encrypting.key);
        ASSERT_EQ(client_decrypting.salt, server_encrypting.salt);
    }

protected:
    crypto::Certificate _client_certificate;
    crypto::Certificate _server_certificate;

    std::optional<Session> _client;
    std::optional<Session> _server;

    Session::Options _client_options{
        .type = Session::Type::kClient,
        .srtp_profiles = etl::vector<Session::SrtpProfile, 2>{
            Session::SrtpProfile::kAes128CmSha1_80,
            Session::SrtpProfile::kAes128CmSha1_32
        },
        .remote_peer_cert_digest = {},
        .log_ctx = "[client] "
    };
    Session::Options _server_options{
        .type = Session::Type::kServer,
        .srtp_profiles = etl::vector<Session::SrtpProfile, 2>{
            Session::SrtpProfile::kAes128CmSha1_80,
            Session::SrtpProfile::kAes128CmSha1_32
        },
        .remote_peer_cert_digest = {},
        .log_ctx = "[server] "
    };

    etl::queue<std::pair<bool, Buffer>, 32> _queue;
    etl::vector<Session::State, 8> _client_states;
    etl::vector<Session::State, 8> _server_states;
};

TEST_F(SessionTest, Basic) {
    Process();

    ASSERT_NO_FATAL_FAILURE(AssertStates(_client_states, {Session::State::kConnecting, Session::State::kConnected}));
    ASSERT_NO_FATAL_FAILURE(AssertStates(_server_states, {Session::State::kConnecting, Session::State::kConnected}));
    ASSERT_NO_FATAL_FAILURE(AssertSrtpProfile(Session::SrtpProfile::kAes128CmSha1_80));
    ASSERT_NO_FATAL_FAILURE(AssertKeyingMaterial());
    ASSERT_NO_FATAL_FAILURE(AssertSendData());

    _client->Stop();
    _server->Stop();
    Process();
}

TEST_F(SessionTest, WithRemoteCertificateValidation) {
    auto server_cert = _server_certificate.GetDigestSha256String();
    auto client_cert = _client_certificate.GetDigestSha256String();
    _client_options.remote_peer_cert_digest = server_cert;
    _server_options.remote_peer_cert_digest = client_cert;
    Init();

    Process();

    ASSERT_NO_FATAL_FAILURE(AssertStates(_client_states, {Session::State::kConnecting, Session::State::kConnected}));
    ASSERT_NO_FATAL_FAILURE(AssertStates(_server_states, {Session::State::kConnecting, Session::State::kConnected}));
    ASSERT_NO_FATAL_FAILURE(AssertSrtpProfile(Session::SrtpProfile::kAes128CmSha1_80));
    ASSERT_NO_FATAL_FAILURE(AssertKeyingMaterial());
    ASSERT_NO_FATAL_FAILURE(AssertSendData());

    _client->Stop();
    _server->Stop();
    Process();
}

TEST_F(SessionTest, SelectNonDefaultSrtpProfile) {
    _client_options.srtp_profiles = etl::vector<Session::SrtpProfile, 2>{
        Session::SrtpProfile::kAes128CmSha1_32
    };
    auto server_cert = _server_certificate.GetDigestSha256String();
    auto client_cert = _client_certificate.GetDigestSha256String();
    _client_options.remote_peer_cert_digest = server_cert;
    _server_options.remote_peer_cert_digest = client_cert;
    Init();

    Process();

    ASSERT_NO_FATAL_FAILURE(AssertStates(_client_states, {Session::State::kConnecting, Session::State::kConnected}));
    ASSERT_NO_FATAL_FAILURE(AssertStates(_server_states, {Session::State::kConnecting, Session::State::kConnected}));
    ASSERT_NO_FATAL_FAILURE(AssertSrtpProfile(Session::SrtpProfile::kAes128CmSha1_32));
    ASSERT_NO_FATAL_FAILURE(AssertKeyingMaterial());
    ASSERT_NO_FATAL_FAILURE(AssertSendData());

    _client->Stop();
    _server->Stop();
    Process();
}

TEST_F(SessionTest, PacketLoss) {
    _client->Process();
    _queue.pop();

    while(true) {
        auto timeout = _client->GetTimeout();
        if(!timeout) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::nanoseconds(*timeout));
        _client->Process();
        _server->Process();
        Process();
    }

    ASSERT_NO_FATAL_FAILURE(AssertStates(_client_states, {Session::State::kConnecting, Session::State::kConnected}));
    ASSERT_NO_FATAL_FAILURE(AssertStates(_server_states, {Session::State::kConnecting, Session::State::kConnected}));
    ASSERT_NO_FATAL_FAILURE(AssertSrtpProfile(Session::SrtpProfile::kAes128CmSha1_80));
    ASSERT_NO_FATAL_FAILURE(AssertKeyingMaterial());
    ASSERT_NO_FATAL_FAILURE(AssertSendData());

    _client->Stop();
    _server->Stop();
    Process();
}

TEST_F(SessionTest, DISABLED_FailOnPacketLoss_BigTimeout) {
    _client->Process();
    _queue.pop();

    while(true) {
        auto timeout = _client->GetTimeout();
        if(!timeout) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::nanoseconds(*timeout));
        _client->Process();
        _server->Process();
        _queue.pop();
        Process();
    }

    ASSERT_NO_FATAL_FAILURE(AssertStates(_client_states, {Session::State::kConnecting, Session::State::kFailed}));
    ASSERT_TRUE(_server_states.empty());

    ASSERT_FALSE(_client->GetSrtpProfile().has_value());
    ASSERT_FALSE(_server->GetSrtpProfile().has_value());

    _client->Stop();
    _server->Stop();
    Process();
}

TEST_F(SessionTest, WrongRemoteCertificate) {
    crypto::Certificate unknown_certificate;
    auto unknown_cert = unknown_certificate.GetDigestSha256String();
    auto client_cert = _client_certificate.GetDigestSha256String();
    _client_options.remote_peer_cert_digest = unknown_cert;
    _server_options.remote_peer_cert_digest = client_cert;
    Init();

    Process();

    ASSERT_NO_FATAL_FAILURE(AssertStates(_client_states, {Session::State::kConnecting, Session::State::kFailed}));
    ASSERT_NO_FATAL_FAILURE(AssertStates(_server_states, {Session::State::kConnecting, Session::State::kFailed}));

    ASSERT_FALSE(_client->GetSrtpProfile().has_value());
    ASSERT_FALSE(_server->GetSrtpProfile().has_value());

    _client->Stop();
    _server->Stop();
    Process();
}

}
