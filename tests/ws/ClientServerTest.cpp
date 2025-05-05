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

public:
    ClientServerTest()
        : _io(std::thread::hardware_concurrency())
        , _ca(crypto::Certificate::Options{
            .cert = std::string{PROJECT_SOURCE_DIR} + "/data/ca.crt",
            .key  = std::string{PROJECT_SOURCE_DIR} + "/data/ca.key"
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
    Server server(Server::Options{kLocalHost, kWsPortTest}, _io.GetExecutor(), *_server_ssl_ctx);
    server.SetOnNewConnectionCallback([](ConnectionPtr connection) {
        connection->SetProcessMessageCallback([](std::string&& request) -> std::string {
            return request;
        });
    });
    server.Start();

    Event on_ready;
    Event on_done;

    auto client = std::make_shared<Client>(Client::Options{kLocalHost, kWsPortTest}, _io.GetExecutor(), *_client_ssl_ctx);
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

TEST_F(ClientServerTest, CloseConnection) {
    Server server(Server::Options{kLocalHost, kWsPortTest}, _io.GetExecutor(), *_server_ssl_ctx);
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
    auto client = std::make_shared<Client>(Client::Options{kLocalHost, kWsPortTest}, _io.GetExecutor(), *_client_ssl_ctx);
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
    Server server(Server::Options{kLocalHost, kWsPortTest}, _io.GetExecutor(), *_server_ssl_ctx);
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
        auto client = std::make_shared<Client>(Client::Options{kLocalHost, kWsPortTest}, _io.GetStrand(), *_client_ssl_ctx);
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
