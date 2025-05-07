#pragma once

#include "tau/ws/Connection.h"
#include <list>

namespace tau::ws {

class Server {
public:
    struct Dependencies {
        Executor executor;
    };

    struct Options {
        std::string host;
        uint16_t port;
        SslContext& ssl_ctx;
    };

    using OnNewConnectionCallback = std::function<void(ConnectionPtr)>;

public:
    Server(Dependencies&& deps, Options&& options);
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
    asio_tcp::acceptor _acceptor;
    std::list<ConnectionWeakPtr> _connections;

    OnNewConnectionCallback _on_new_connection_callback;
};

}
