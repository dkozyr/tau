#include "tau/http/Server.h"
#include "tau/http/Client.h"
#include "tau/asio/Ssl.h"
#include "tau/asio/ThreadPool.h"
#include "tau/crypto/Certificate.h"
#include "tests/lib/Common.h"

namespace tau::http {

class ClientServerTest : public ::testing::Test {
public:
    static constexpr uint16_t kHttpPort  = 8888;
    static constexpr uint16_t kHttpsPort = 8889;

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
        , _server_ssl_context(CreateSslContextPtr(
            _server_certificate.GetCertificateBuffer(), _server_certificate.GetPrivateKeyBuffer()))
    {}

    ~ClientServerTest() {
        _server.reset();
        _io.Join();
    }

    void InitServerAndStart(bool tls) {
        InitServer(tls);
        InitServerCallback();
        _server->Start();
    }

    void InitServer(bool tls) {
        _server.emplace(
            Server::Dependencies{.executor = _io.GetExecutor()},
            Server::Options{
                .port = tls ? kHttpsPort : kHttpPort,
                .ssl_ctx = tls ? std::move(_server_ssl_context) : nullptr
            });
    }

    void InitServerCallback() {
        _server->SetRequestCallback(
            [this](const beast_request& request, const Server::ResponseCallback& callback) {
                std::string target = request.target();
                if(auto pos = target.find('?'); pos != std::string::npos) {
                    target.resize(pos);
                }
                try {
                    if((request.method() == beast_http::verb::get) && (target == "/")) {
                        TAU_LOG_DEBUG("Request: " << request);
                        _requests_counter.fetch_add(1);

                        beast_response response{beast_http::status::ok, request.version()};
                        response.set(beast_http::field::content_type, "text/plain");
                        beast::ostream(response.body())
                            << "Requests: " << _requests_counter << "\n";
                        response.content_length(response.body().size());
                        TAU_LOG_DEBUG("Response: " << response);

                        callback(std::move(response));
                    } else {
                        beast_response response{beast_http::status::not_found, request.version()};
                        callback(std::move(response));
                    }
                } catch(const std::exception& e) {
                    TAU_LOG_WARNING("Exception: " << e.what());
                    beast_response bad_request{beast::http::status::bad_request, request.version()};
                    return callback(std::move(bad_request));
                }
            });
    }

protected:
    ThreadPool _io;

    crypto::Certificate _ca;
    crypto::Certificate _server_certificate;
    crypto::Certificate _client_certificate;
    SslContextPtr _server_ssl_context;

    std::optional<Server> _server;
    std::atomic<size_t> _requests_counter = 0;
};

TEST_F(ClientServerTest, BasicHttp) {
    InitServerAndStart(false);

    Event has_response;
    Client::Create(_io.GetExecutor(),
        Client::Options{
            .method = beast_http::verb::get,
            .host = "127.0.0.1",
            .port = kHttpPort,
            .target = "/",
            .body = {},
            .fields = {
                {beast_http::field::user_agent, "tau"}
            },
            .ssl_ctx = nullptr
        },
        [&](boost_ec, const beast_response& response) {
            ASSERT_EQ("Requests: 1\n", beast::buffers_to_string(response.body().data()) );
            has_response.Set();
        }
    );
    ASSERT_TRUE(has_response.WaitFor(100ms));
}

TEST_F(ClientServerTest, BasicHttps) {
    InitServerAndStart(true);

    Event has_response;
    Client::Create(_io.GetExecutor(),
        Client::Options{
            .method = beast_http::verb::get,
            .host = "127.0.0.1",
            .port = kHttpsPort,
            .target = "/",
            .body = {},
            .fields = {
                {beast_http::field::user_agent, "tau"}
            },
            .ssl_ctx = CreateSslContextPtr(_client_certificate.GetCertificateBuffer(), _client_certificate.GetPrivateKeyBuffer())
        },
        [&](boost_ec, const beast_response& response) {
            ASSERT_EQ("Requests: 1\n", beast::buffers_to_string(response.body().data()) );
            has_response.Set();
        }
    );
    ASSERT_TRUE(has_response.WaitFor(100ms));
}

TEST_F(ClientServerTest, HttpsWithValidatingClientCertificate) {
    _server_ssl_context->load_verify_file(kCaCertPath);
    _server_ssl_context->set_verify_mode(asio_ssl::verify_peer | asio_ssl::verify_fail_if_no_peer_cert);
    InitServerAndStart(true);

    Event has_response;
    Client::Create(_io.GetExecutor(),
        Client::Options{
            .method = beast_http::verb::get,
            .host = "127.0.0.1",
            .port = kHttpsPort,
            .target = "/",
            .body = {},
            .fields = {
                {beast_http::field::user_agent, "tau"}
            },
            .ssl_ctx = CreateSslContextPtr(_client_certificate.GetCertificateBuffer(), _client_certificate.GetPrivateKeyBuffer())
        },
        [&](boost_ec, const beast_response& response) {
            ASSERT_EQ("Requests: 1\n", beast::buffers_to_string(response.body().data()) );
            has_response.Set();
        }
    );
    ASSERT_TRUE(has_response.WaitFor(100ms));
}

TEST_F(ClientServerTest, HttpsClientWithWrongCertificate) {
    _server_ssl_context->load_verify_file(kCaCertPath);
    _server_ssl_context->set_verify_mode(asio_ssl::verify_peer | asio_ssl::verify_fail_if_no_peer_cert);
    InitServerAndStart(true);

    crypto::Certificate wrong_ca;
    crypto::Certificate cert_wrong_ca(crypto::Certificate::OptionsSelfSigned{.ca = wrong_ca});

    Event has_response;
    Client::Create(_io.GetExecutor(),
        Client::Options{
            .method = beast_http::verb::get,
            .host = "127.0.0.1",
            .port = kHttpsPort,
            .target = "/",
            .body = {},
            .fields = {
                {beast_http::field::user_agent, "tau"}
            },
            .ssl_ctx = CreateSslContextPtr(cert_wrong_ca.GetCertificateBuffer(), cert_wrong_ca.GetPrivateKeyBuffer())
        },
        [&](boost_ec ec, const beast_response& response) {
            ASSERT_NE(0, ec.value());
            ASSERT_EQ("tlsv1 alert unknown ca (SSL routines)", ec.message());
            ASSERT_EQ(0, response.body().size());
            has_response.Set();
        }
    );
    ASSERT_TRUE(has_response.WaitFor(100ms));
}

TEST_F(ClientServerTest, DISABLED_MANUAL_Localhost) {
    InitServerAndStart(true);

    Client::Create(_io.GetExecutor(),
        Client::Options{
            .method = beast_http::verb::get,
            .host = "127.0.0.1",
            .port = 8889,
            .target = "/",
            .body = {},
            .fields = {
                {beast_http::field::user_agent, "tau"}
            },
            .ssl_ctx = CreateSslContextPtr(_client_certificate.GetCertificateBuffer(), _client_certificate.GetPrivateKeyBuffer())
        },
        [](boost_ec ec, const beast_response& response) {
            if(!ec) {
                TAU_LOG_INFO("Response: " << response.body().size());
            } else {
                TAU_LOG_WARNING("Client error: " << ec.message());
            }
        }
    );

    Event().WaitFor(60s);
}

}
