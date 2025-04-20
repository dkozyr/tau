
#pragma once

#include "tau/asio/Ssl.h"
#include "tau/asio/Timer.h"
#include "tau/common/Variant.h"

namespace tau::http {

class Client : public std::enable_shared_from_this<Client> {
public:
    static constexpr auto kTimeoutDefault = std::chrono::seconds(10);

    struct HttpField {
        using Type = std::variant<beast_http::field, std::string>;

        Type type;
        std::string value;
    };

    struct Options {
        beast_http::verb method;
        std::string host;
        uint16_t port;
        std::string target;
        std::string body;
        std::vector<HttpField> fields;

        SslContextPtr ssl_ctx = nullptr;
    };

    using ResponseCallback = std::function<void(boost_ec ec, const beast_response& response)>;

    using Socket = std::variant<SslSocket, asio_tcp::socket>;

public:
    static void Create(Executor executor, Options&& options, ResponseCallback callback) {
        std::shared_ptr<Client> self(new Client(executor, std::move(options), std::move(callback)));
        self->Start();
        return;
    }

private:
    Client(Executor executor, Options&& options, ResponseCallback&& callback);

    void Start();
    void OnResolve(boost_ec ec, asio_tcp::resolver::results_type results);
    void OnConnect(boost_ec ec);
    void OnHandshake(boost_ec ec);
    void OnWrite(boost_ec ec, size_t bytes);
    void OnRead(boost_ec ec, size_t bytes);
    void Shutdown();
    void OnShutdown(beast_ec ec);
    void OnTimeout(beast_ec ec);

    static Socket CreateSocket(Executor executor, const SslContextPtr& ssl_ctx);

private:
    const Options _options;
    ResponseCallback _response_callback;
    asio_tcp::resolver _resolver;
    Socket _socket;
    Timer _timeout;

    beast_request _request;
    beast_response _response;
    beast::flat_buffer _buffer;
};

}
