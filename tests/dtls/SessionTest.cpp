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
            Session::Dependencies{.clock = _clock, .udp_allocator = g_udp_allocator, .certificate = _client_certificate},
            Session::Options{_client_options});
        _server.emplace(
            Session::Dependencies{.clock = _clock, .udp_allocator = g_udp_allocator, .certificate = _server_certificate},
            Session::Options{_server_options});

        _client->SetSendCallback([&](Buffer&& packet) { _queue.push(std::make_pair(false, std::move(packet))); });
        _server->SetSendCallback([&](Buffer&& packet) { _queue.push(std::make_pair(true, std::move(packet))); });

        _client->SetRecvCallback([&](Buffer&& packet) {
            auto view = packet.GetView();
            auto message = std::string_view{reinterpret_cast<char*>(view.ptr), view.size};
            TAU_LOG_INFO("[client] [recv] message: " << message);
            EXPECT_EQ(message, "Hello from server!");
        });
        _server->SetRecvCallback([&](Buffer&& packet) {
            auto view = packet.GetView();
            auto message = std::string_view{reinterpret_cast<char*>(view.ptr), view.size};
            TAU_LOG_INFO("[server] [recv] message: " << message);
            EXPECT_EQ(message, "Hello from client!");
        });

        _client->SetStateCallback([&](Session::State state) { _client_states.push_back(state); });
        _server->SetStateCallback([&](Session::State state) { _server_states.push_back(state); });
    }

    void Process() {
        while(!_queue.empty()) {
            auto& [from_server, packet] = _queue.front();
            if(from_server) {
                _client->Recv(std::move(packet));
            } else {
                _server->Recv(std::move(packet));
            }
            _queue.pop();

            _client->Process();
            _server->Process();
        }
    }

    static void AssertStates(const std::vector<Session::State>& actual, const std::vector<Session::State>& target) {
        ASSERT_EQ(actual.size(), target.size());
        for(size_t i = 0; i < actual.size(); ++i) {
            ASSERT_EQ(target[i], actual[i]);
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
        ASSERT_FALSE(client_encrypting.empty());
        ASSERT_FALSE(server_encrypting.empty());
        ASSERT_EQ(client_encrypting, server_decrypting);
        ASSERT_EQ(client_decrypting, server_encrypting);
    }

protected:
    TestClock _clock;

    Certificate _client_certificate;
    Certificate _server_certificate;

    std::optional<Session> _client;
    std::optional<Session> _server;

    Session::Options _client_options{
        .type = Session::Type::kClient,
        .srtp_profiles = Session::kSrtpProfilesDefault,
        .remote_peer_cert_digest = {},
        .log_ctx = "[client] "
    };
    Session::Options _server_options{
        .type = Session::Type::kServer,
        .srtp_profiles = Session::kSrtpProfilesDefault,
        .remote_peer_cert_digest = {},
        .log_ctx = "[server] "
    };

    std::queue<std::pair<bool, Buffer>> _queue;
    std::vector<Session::State> _client_states;
    std::vector<Session::State> _server_states;
};

TEST_F(SessionTest, Basic) {
    _client->Process();
    Process();

    ASSERT_NO_FATAL_FAILURE(AssertStates(_client_states, {Session::State::kConnecting, Session::State::kConnected}));
    ASSERT_NO_FATAL_FAILURE(AssertStates(_server_states, {Session::State::kConnecting, Session::State::kConnected}));
    ASSERT_NO_FATAL_FAILURE(AssertSrtpProfile(Session::SrtpProfile::kAes128CmSha1_80));
    ASSERT_NO_FATAL_FAILURE(AssertKeyingMaterial());

    const char* client_message = "Hello from client!";
    ASSERT_TRUE(_client->Send(BufferViewConst{.ptr = (const uint8_t*)client_message, .size = strlen(client_message)}));
    const char* server_message = "Hello from server!";
    ASSERT_TRUE(_server->Send(BufferViewConst{.ptr = (const uint8_t*)server_message, .size = strlen(server_message)}));
    Process();

    _client->Stop();
    _server->Stop();
    Process();
}

TEST_F(SessionTest, WithRemoteCertificateValidation) {
    _client_options.remote_peer_cert_digest = _server_certificate.GetDigestSha256String();
    _server_options.remote_peer_cert_digest = _client_certificate.GetDigestSha256String();
    Init();

    _client->Process();
    Process();

    ASSERT_NO_FATAL_FAILURE(AssertStates(_client_states, {Session::State::kConnecting, Session::State::kConnected}));
    ASSERT_NO_FATAL_FAILURE(AssertStates(_server_states, {Session::State::kConnecting, Session::State::kConnected}));
    ASSERT_NO_FATAL_FAILURE(AssertSrtpProfile(Session::SrtpProfile::kAes128CmSha1_80));
    ASSERT_NO_FATAL_FAILURE(AssertKeyingMaterial());

    const char* client_message = "Hello from client!";
    ASSERT_TRUE(_client->Send(BufferViewConst{.ptr = (const uint8_t*)client_message, .size = strlen(client_message)}));
    const char* server_message = "Hello from server!";
    ASSERT_TRUE(_server->Send(BufferViewConst{.ptr = (const uint8_t*)server_message, .size = strlen(server_message)}));
    Process();

    _client->Stop();
    _server->Stop();
    Process();
}

TEST_F(SessionTest, SelectNonDefaultSrtpProfile) {
    _client_options.srtp_profiles = "SRTP_AES128_CM_SHA1_32";
    _client_options.remote_peer_cert_digest = _server_certificate.GetDigestSha256String();
    _server_options.remote_peer_cert_digest = _client_certificate.GetDigestSha256String();
    Init();

    _client->Process();
    Process();

    ASSERT_NO_FATAL_FAILURE(AssertStates(_client_states, {Session::State::kConnecting, Session::State::kConnected}));
    ASSERT_NO_FATAL_FAILURE(AssertStates(_server_states, {Session::State::kConnecting, Session::State::kConnected}));
    ASSERT_NO_FATAL_FAILURE(AssertSrtpProfile(Session::SrtpProfile::kAes128CmSha1_32));
    ASSERT_NO_FATAL_FAILURE(AssertKeyingMaterial());

    const char* client_message = "Hello from client!";
    ASSERT_TRUE(_client->Send(BufferViewConst{.ptr = (const uint8_t*)client_message, .size = strlen(client_message)}));
    const char* server_message = "Hello from server!";
    ASSERT_TRUE(_server->Send(BufferViewConst{.ptr = (const uint8_t*)server_message, .size = strlen(server_message)}));
    Process();

    _client->Stop();
    _server->Stop();
    Process();
}

TEST_F(SessionTest, WrongRemoteCertificate) {
    Certificate unknown_certificate;
    _client_options.remote_peer_cert_digest = unknown_certificate.GetDigestSha256String();
    _server_options.remote_peer_cert_digest = _client_certificate.GetDigestSha256String();
    Init();

    _client->Process();
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
