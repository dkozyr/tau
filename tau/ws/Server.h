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
        http::Fields http_fields = {};
    };

    using ValidateRequestCallback = Connection::ValidateRequestCallback;
    using OnNewConnectionCallback = std::function<void(ConnectionPtr)>;

public:
    Server(Dependencies&& deps, Options&& options);
    ~Server();

    void SetValidateRequestCallback(ValidateRequestCallback callback) { _validate_request_callback = std::move(callback); }
    void SetOnNewConnectionCallback(OnNewConnectionCallback callback) { _on_new_connection_callback = std::move(callback); }

    void Start();

private:
    void DoAccept();
    void OnAccept(beast_ec ec, asio_tcp::socket socket);

    bool ValidateRequest(const beast_request& request) const;
    void ProcessResponse(beast_ws::response_type& response) const;

    void CloseConnections();
    void ClearConnections();

private:
    Executor _executor;
    Options _options;
    asio_tcp::acceptor _acceptor;
    std::list<ConnectionWeakPtr> _connections;

    ValidateRequestCallback _validate_request_callback;
    OnNewConnectionCallback _on_new_connection_callback;
};

}
