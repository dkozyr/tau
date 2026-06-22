
#pragma once

#include "tau/asio/Ssl.h"
#include "tau/asio/Timer.h"
#include "tau/http/Field.h"
#include "tau/common/Variant.h"
#include <etl/string_view.h>

namespace tau::http {

class Client : public std::enable_shared_from_this<Client> {
    using Resolver = asio::ip::tcp::resolver;
    using Socket = asio::ip::tcp::socket;
    using SocketVar = std::variant<SslSocket, Socket>;

public:
    static constexpr auto kTimeoutDefault = std::chrono::seconds(10);

    struct Options {
        beast_http::verb method;
        etl::string_view host;
        uint16_t port;
        etl::string_view target;
        etl::string_view body;
        Fields fields;

        SslContextPtr ssl_ctx = nullptr;
    };

    using ResponseCallback = std::function<void(boost_ec ec, const beast_response& response)>;

public:
    static void Create(Executor executor, Options&& options, ResponseCallback callback) {
        std::shared_ptr<Client> self(new Client(executor, std::move(options), std::move(callback)));
        self->Start();
        return;
    }

private:
    Client(Executor executor, Options&& options, ResponseCallback&& callback);

    void Start();
    void OnResolve(boost_ec ec, Resolver::results_type results);
    void OnConnect(boost_ec ec);
    void OnHandshake(boost_ec ec);
    void OnWrite(boost_ec ec, size_t bytes);
    void OnRead(boost_ec ec, size_t bytes);
    void Shutdown();
    void OnShutdown(beast_ec ec);
    void OnTimeout(beast_ec ec);

    static SocketVar CreateSocket(Executor executor, const SslContextPtr& ssl_ctx);

private:
    const Options _options;
    ResponseCallback _response_callback;
    Resolver _resolver;
    SocketVar _socket;
    Timer _timeout;

    beast_request _request;
    beast_response _response;
    beast::flat_buffer _buffer;
};

}
