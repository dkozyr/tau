#pragma once

#include "tau/ws/Connection.h"
#include <list>

namespace tau::ws {

class Server {
public:
    using OnNewConnectionCallback = std::function<void(ConnectionPtr)>;

    struct Options{
        std::string host;
        uint16_t port;
    };

public:
    Server(Options&& options, Executor executor, SslContext& ssl_ctx);
    ~Server();

    void SetOnNewConnectionCallback(OnNewConnectionCallback&& callback) { _on_new_connection_callback = std::move(callback); }

    void Start();

private:
    void DoAccept();
    void OnAccept(beast_ec ec, asio_tcp::socket socket);

    void CloseConnections();
    void ClearConnections();

private:
    Executor _executor;
    SslContext& _ssl_ctx;
    // asio_ssl::context& _ssl_ctx;
    asio_tcp::acceptor _acceptor;
    std::list<ConnectionWeakPtr> _connections;

    OnNewConnectionCallback _on_new_connection_callback;
};

}
