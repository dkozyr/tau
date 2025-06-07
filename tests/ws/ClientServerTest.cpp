#include "tau/ws/Server.h"
#include "tau/ws/Client.h"
#include "tau/asio/ThreadPool.h"
#include "tau/crypto/Certificate.h"
#include "tests/lib/Common.h"

namespace tau::ws {

class ClientServerTest : public ::testing::Test {
public:
    static inline const std::string kLocalHost = "127.0.0.1";
    static inline const uint16_t kWsPortTest = 12345;

    static inline const std::string kCaCertPath = std::string{PROJECT_SOURCE_DIR} + "/data/keys/ca.crt";
    static inline const std::string kCaKeyPath  = std::string{PROJECT_SOURCE_DIR} + "/data/keys/ca.key";

public:
    ClientServerTest()
        : _io(std::thread::hardware_concurrency())
        , _ca(crypto::Certificate::Options{
            .cert = kCaCertPath,
            .key  = kCaKeyPath
        })
        , _server_certificate(crypto::Certificate::OptionsSelfSigned{.ca = _ca})
        , _client_certificate(crypto::Certificate::OptionsSelfSigned{.ca = _ca})
        , _server_ssl_ctx(CreateSslContextPtr(
            _server_certificate.GetCertificateBuffer(), _server_certificate.GetPrivateKeyBuffer()))
        , _client_ssl_ctx(CreateSslContextPtr(
            _server_certificate.GetCertificateBuffer(), _server_certificate.GetPrivateKeyBuffer()))
    {}

    ~ClientServerTest() {
        _io.Join();
    }

protected:
    ThreadPool _io;
    crypto::Certificate _ca;
    crypto::Certificate _server_certificate;
    crypto::Certificate _client_certificate;
    SslContextPtr _server_ssl_ctx;
    SslContextPtr _client_ssl_ctx;
};

TEST_F(ClientServerTest, Basic) {
    Server server(
        Server::Dependencies{.executor = _io.GetExecutor()},
        Server::Options{kLocalHost, kWsPortTest, *_server_ssl_ctx}
    );
    server.SetOnNewConnectionCallback([](ConnectionPtr connection) {
        connection->SetProcessMessageCallback([](std::string&& request) -> std::string {
            return request;
        });
    });
    server.Start();

    Event on_ready;
    Event on_done;

    auto client = std::make_shared<Client>(_io.GetExecutor(), Client::Options{kLocalHost, kWsPortTest, *_client_ssl_ctx});
    client->SetOnConnectedCallback([&on_ready]() {
        on_ready.Set();
    });
    client->SetOnMessageCallback([&on_done](std::string&& message) {
        TAU_LOG_INFO("[client] incoming message: " << message);
        on_done.Set();
    });
    client->Start();
    ASSERT_TRUE(on_ready.WaitFor(1s));

    client->PostMessage("Hello world");
    ASSERT_TRUE(on_done.WaitFor(1s));
    client.reset();
}

TEST_F(ClientServerTest, ServerValidateRequestOrigin) {
    Server server(
        Server::Dependencies{.executor = _io.GetExecutor()},
        Server::Options{kLocalHost, kWsPortTest, *_server_ssl_ctx, http::Fields{
            http::Field{beast_http::field::server, "example.com"},
            http::Field{beast_http::field::access_control_allow_origin, "https://www.example.com"},
            http::Field{"custom-name", "custom-value"},
        }}
    );
    server.SetValidateRequestCallback([](const beast_request& request) {
        TAU_LOG_INFO("Request: " << request);
        const auto it_origin = request.find("Origin");
        if(it_origin == request.end()) {
            TAU_LOG_WARNING("No Origin header");
            return false;
        }
        if(it_origin->value() != "example.com") {
            TAU_LOG_WARNING("Wrong Origin: " << it_origin->value());
            return false;
        }
        return true;
    });
    server.SetOnNewConnectionCallback([](ConnectionPtr connection) {
        connection->SetProcessMessageCallback([](std::string&& request) -> std::string {
            return request;
        });
    });
    server.Start();

    {
        Event on_ready;
        Event on_done;

        auto good_client = std::make_shared<Client>(_io.GetExecutor(),
            Client::Options{kLocalHost, kWsPortTest, *_client_ssl_ctx, {
                http::Field{beast_http::field::origin, "example.com"},
                http::Field{beast_http::field::host, "https://www.example.com:443"},
                http::Field{"custom-name", "custom-value"},
            }
        });
        good_client->SetOnConnectedCallback([&on_ready]() {
            on_ready.Set();
        });
        good_client->SetOnMessageCallback([&on_done](std::string&& message) {
            TAU_LOG_INFO("[client] incoming message: " << message);
            on_done.Set();
        });
        good_client->Start();
        ASSERT_TRUE(on_ready.WaitFor(1s));

        good_client->PostMessage("Hello world");
        ASSERT_TRUE(on_done.WaitFor(1s));
    }
    {
        Event on_ready;

        auto bad_client = std::make_shared<Client>(_io.GetExecutor(),
            Client::Options{kLocalHost, kWsPortTest, *_client_ssl_ctx, {
                http::Field{beast_http::field::origin, "wrong-origin-domain.com"},
                http::Field{beast_http::field::host, "https://www.example.com:443"},
                http::Field{"custom-name", "custom-value"},
            }
        });
        bad_client->SetOnConnectedCallback([&on_ready]() {
            on_ready.Set();
        });
        bad_client->SetOnMessageCallback([](std::string&&) {
            ASSERT_TRUE(false);
        });
        bad_client->Start();
        ASSERT_FALSE(on_ready.WaitFor(100ms));
    }
}

TEST_F(ClientServerTest, CloseConnection) {
    Server server(
        Server::Dependencies{.executor = _io.GetExecutor()},
        Server::Options{kLocalHost, kWsPortTest, *_server_ssl_ctx}
    );
    ConnectionPtr connection_ptr = nullptr;
    server.SetOnNewConnectionCallback([&connection_ptr](ConnectionPtr connection) {
        connection->SetProcessMessageCallback([](std::string&& request) -> std::string {
            return request;
        });
        connection_ptr = std::move(connection);
    });
    server.Start();

    Event on_ready;
    Event on_done;
    auto client = std::make_shared<Client>(_io.GetExecutor(), Client::Options{kLocalHost, kWsPortTest, *_client_ssl_ctx});
    client->SetOnConnectedCallback([&on_ready]() {
        on_ready.Set();
    });
    client->SetOnMessageCallback([&on_done](std::string&& message) {
        TAU_LOG_INFO("[client] incoming message: " << message);
        on_done.Set();
    });
    client->Start();
    ASSERT_TRUE(on_ready.WaitFor(1s));

    connection_ptr->Close();
    client->PostMessage("Hello world");
    ASSERT_FALSE(on_done.WaitFor(100ms));
    client.reset();
}

TEST_F(ClientServerTest, SeveralClients) {
    Server server(
        Server::Dependencies{.executor = _io.GetExecutor()},
        Server::Options{kLocalHost, kWsPortTest, *_server_ssl_ctx}
    );
    server.SetOnNewConnectionCallback([](ConnectionPtr connection) {
        connection->SetProcessMessageCallback([](std::string&& request) -> std::string {
            return request;
        });
    });
    server.Start();

    Event done;
    std::atomic<size_t> clients_done{0};
    constexpr auto kTestClients = 42;
    std::vector<std::shared_ptr<Client>> clients;
    for(size_t i = 0; i < kTestClients; ++i) {
        auto client = std::make_shared<Client>(_io.GetExecutor(), Client::Options{kLocalHost, kWsPortTest, *_client_ssl_ctx});
        client->SetOnConnectedCallback([weak_self = std::weak_ptr<Client>(client)]() {
            if(auto self = weak_self.lock()) {
                self->PostMessage("Hello world");
            }
        });
        client->SetOnMessageCallback([&done, &clients_done](std::string&&) {
            clients_done.fetch_add(1, std::memory_order_seq_cst);
            if(clients_done.load() == kTestClients) {
                done.Set();
            }
        });
        client->Start();
        clients.push_back(std::move(client));
    }

    EXPECT_TRUE(done.WaitFor(2s)) << "clients_done: " << clients_done << "/" << kTestClients;
    clients.clear();
}

}
