#include "apps/signalling-server/Server.h"
#include "Device.h"
#include "Client.h"
#include "tau/ws/Server.h"
#include "tau/ws/Client.h"
#include "tau/asio/ThreadPool.h"
#include "tau/asio/ToString.h"
#include "tau/crypto/Certificate.h"
#include "tests/lib/Common.h"

namespace tau::signalling {

class ServerTest : public ::testing::Test {
public:
    static inline const etl::string_view kLocalHost = "127.0.0.1";
    static inline const uint16_t kSignallingPort = 12345;

    static inline const char kCaCertPath[] = PROJECT_SOURCE_DIR "/data/keys/ca.crt";
    static inline const char kCaKeyPath[]  = PROJECT_SOURCE_DIR "/data/keys/ca.key";

public:
    ServerTest()
        : _io(std::thread::hardware_concurrency())
        , _ca(crypto::Certificate::Options{
            .cert = etl::string_view{kCaCertPath},
            .key  = etl::string_view{kCaKeyPath}
        })
        , _server_certificate(crypto::Certificate::OptionsSelfSigned{.ca = _ca})
        , _client_certificate(crypto::Certificate::OptionsSelfSigned{.ca = _ca})
        , _server_ssl_ctx(CreateSslContextPtr(
            _server_certificate.GetCertificateBuffer(), _server_certificate.GetPrivateKeyBuffer()))
        , _client_ssl_ctx(CreateSslContextPtr(
            _client_certificate.GetCertificateBuffer(), _client_certificate.GetPrivateKeyBuffer()))
    {
        Init();
    }

    ~ServerTest() {
        _io.Join();
    }

    void Init() {
        _server.Start(Server::Options{kLocalHost, kSignallingPort, *_server_ssl_ctx});
    }

protected:
    ThreadPool _io;
    SteadyClock _clock;
    crypto::Certificate _ca;
    crypto::Certificate _server_certificate;
    crypto::Certificate _client_certificate;
    SslContextPtr _server_ssl_ctx;
    SslContextPtr _client_ssl_ctx;

    Server _server;
};

TEST_F(ServerTest, Basic) {
    Device device(
        Device::Dependencies{
            .executor = _io.GetExecutor(),
            .clock = _clock,
            .udp_allocator = g_udp_allocator
        },
        Device::Options{
            .device_id = 1234567890,
            .ws_options = ws::Client::Options{kLocalHost, kSignallingPort, "/device", *_client_ssl_ctx}
        });

    std::optional<StreamId> device_stream_id;
    Event on_stream_id_ready;
    device.SetStreamIdCallback([&](StreamId stream_id) {
        device_stream_id = stream_id;
        on_stream_id_ready.Set();
    });
    Event on_device_connected;
    device.SetStateChangeCallback([&](webrtc::State state) {
        if(state == webrtc::State::kConnected) {
            on_device_connected.Set();
        }
    });
    ASSERT_TRUE(device.Start());
    ASSERT_TRUE(on_stream_id_ready.WaitFor(1s));

    {
        Client client(
            Client::Dependencies{
                .executor = _io.GetExecutor(),
                .clock = _clock,
                .udp_allocator = g_udp_allocator
            },
            Client::Options{
                .stream_id = *device_stream_id,
                .ws_options = ws::Client::Options{kLocalHost, kSignallingPort, "/", *_client_ssl_ctx}
            });
        Event on_client_connected;
        client.SetStateChangeCallback([&](webrtc::State state) {
            if(state == webrtc::State::kConnected) {
                on_client_connected.Set();
            }
        });
        ASSERT_TRUE(client.Start());
        ASSERT_TRUE(on_device_connected.WaitFor(3s));
        ASSERT_TRUE(on_client_connected.WaitFor(3s));
        client.Stop();
    }
    {
        Client client(
            Client::Dependencies{
                .executor = _io.GetExecutor(),
                .clock = _clock,
                .udp_allocator = g_udp_allocator
            },
            Client::Options{
                .stream_id = *device_stream_id,
                .ws_options = ws::Client::Options{kLocalHost, kSignallingPort, "/", *_client_ssl_ctx}
            });
        Event on_client_connected;
        client.SetStateChangeCallback([&](webrtc::State state) {
            if(state == webrtc::State::kConnected) {
                on_client_connected.Set();
            }
        });
        ASSERT_TRUE(client.Start());
        ASSERT_TRUE(on_client_connected.WaitFor(3s));
    }
    constexpr auto kTimeout = 2 * std::chrono::milliseconds(Server::kSessionTimeoutMs);
    std::this_thread::sleep_for(kTimeout);
    {
        Client client(
            Client::Dependencies{
                .executor = _io.GetExecutor(),
                .clock = _clock,
                .udp_allocator = g_udp_allocator
            },
            Client::Options{
                .stream_id = *device_stream_id,
                .ws_options = ws::Client::Options{kLocalHost, kSignallingPort, "/", *_client_ssl_ctx}
            });
        Event on_client_connected;
        client.SetStateChangeCallback([&](webrtc::State state) {
            if(state == webrtc::State::kConnected) {
                on_client_connected.Set();
            }
        });
        ASSERT_TRUE(client.Start());
        ASSERT_TRUE(on_client_connected.WaitFor(3s));
    }
}

TEST_F(ServerTest, DISABLED_MANUAL_WebClient) {
    Device device(
        Device::Dependencies{
            .executor = _io.GetExecutor(),
            .clock = _clock,
            .udp_allocator = g_udp_allocator
        },
        Device::Options{
            .device_id = 1234567890,
            .ws_options = ws::Client::Options{kLocalHost, kSignallingPort, "/device", *_client_ssl_ctx}
        });

    std::optional<StreamId> device_stream_id;
    Event on_stream_id_ready;
    device.SetStreamIdCallback([&](StreamId stream_id) {
        device_stream_id = stream_id;
        on_stream_id_ready.Set();
    });
    device.SetStateChangeCallback([&](webrtc::State state) {
        TAU_LOG_INFO("state: " << state);
    });

    ASSERT_TRUE(device.Start());
    ASSERT_TRUE(on_stream_id_ready.WaitFor(1s));

    while(true) {
        std::this_thread::sleep_for(100ms);
    }
}

}
