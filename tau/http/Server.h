#pragma once

#include "tau/http/Connection.h"

namespace tau::http {

class Server {
public:
    struct Dependencies {
        Executor executor;
    };

    struct Options {
        IpAddress local_address = IpAddressV4::any();
        uint16_t port;
        SslContextPtr ssl_ctx = nullptr;
    };

    using RequestCallback = Connection::RequestCallback;
    using ResponseCallback = Connection::ResponseCallback;
    using Socket = Connection::Socket;

public:
    Server(Dependencies&& deps, Options&& options);
    ~Server();

    void SetRequestCallback(RequestCallback callback);
    void Start();

private:
    void Accept();
    void OnAccept(boost_ec ec, Socket socket);

private:
    Executor _executor;
    SslContextPtr _ssl_ctx;
    asio_tcp::acceptor _acceptor;
    RequestCallback _request_callback;
};

}
