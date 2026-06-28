#pragma once

#include "tau/ws/Message.h"
#include "tau/http/Field.h"
#include "tau/asio/Ssl.h"
#include <etl/deque.h>
#include <chrono>

namespace tau::ws {

class Client : public std::enable_shared_from_this<Client> {
public:
    static inline const std::chrono::seconds kHandshakeTimeout{2};
    static inline const std::chrono::seconds kIdleTimeout{30};

    struct Options {
        etl::string_view host;
        uint16_t port;
        etl::string_view path;
        SslContext& ssl_ctx;
        http::Fields http_fields = {};
    };

    using OnConnectedCallback = std::function<void(void)>;
    using OnMessageCallback = std::function<void(String&&)>;
    using OnError = std::function<void(beast_ec ec)>;

public:
    Client(Executor executor, Options&& options);
    ~Client();

    void SetOnConnectedCallback(OnConnectedCallback&& callback) { _on_connected_callback = std::move(callback); }
    void SetOnMessageCallback(OnMessageCallback&& callback) { _on_message_callback = std::move(callback); }
    void SetOnErrorCallback(OnError&& callback) { _on_error_callback = std::move(callback); }

    void Start();
    void PostMessage(String message);
    void Close(bool graceful = true);

private:
    void OnResolve(beast_ec ec, asio::ip::tcp::resolver::results_type results);
    void OnConnect(beast_ec ec, asio::ip::tcp::resolver::results_type::endpoint_type endpoint);
    void OnSslHandshake(beast_ec ec);
    void OnHandshake(beast_ec ec);

    void DoRead();
    void OnRead(beast_ec ec, std::size_t bytes_transferred);
    void DoPostMessage(String&& message);
    void DoWriteLoop();
    void OnWrite(beast_ec ec, size_t bytes_transferred);

    void OnClose(beast_ec ec);

private:
    using SocketType = beast_ws::stream<beast::ssl_stream<beast::tcp_stream>>;

private:
    const Options _options;
    Executor _executor;
    asio::ip::tcp::resolver _resolver;
    SocketType _socket;
    beast_http::response<beast_http::string_body> _response;
    beast::flat_buffer _buffer;
    etl::deque<String, 16> _message_queue;
    bool _closed = false;

    OnConnectedCallback _on_connected_callback;
    OnMessageCallback _on_message_callback;
    OnError _on_error_callback;
};

}
