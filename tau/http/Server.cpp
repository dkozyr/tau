#include "tau/http/Server.h"
#include "tau/common/Log.h"

namespace tau::http {

Server::Server(Dependencies&& deps, Options&& options)
    : _executor(std::move(deps.executor))
    , _ssl_ctx(std::move(options.ssl_ctx))
    , _acceptor(_executor, {options.local_address, options.port}) {
}

Server::~Server() {
    boost_ec ec;
    _acceptor.cancel(ec);
    _acceptor.close(ec);
}

void Server::SetRequestCallback(RequestCallback callback) {
    _request_callback = std::move(callback);
}

void Server::Start() {
    TAU_LOG_INFO("Listening: " << _acceptor.local_endpoint() << (_ssl_ctx ? " (TLS)" : ""));
    Accept();
}

void Server::Accept() {
    if(_ssl_ctx) {
        auto socket = std::make_shared<SslSocket>(_executor, *_ssl_ctx);
        _acceptor.async_accept(socket->lowest_layer(),
            [this, socket](auto ec) mutable {
                OnAccept(ec, std::move(socket));
            });
    } else {
        auto socket = std::make_shared<asio_tcp::socket>(_executor);
        _acceptor.async_accept(*socket,
            [this, socket](auto ec) mutable {
                OnAccept(ec, std::move(socket));
            });
    }
}

void Server::OnAccept(boost_ec ec, Socket socket) {
    if(ec) {
        if((ec == boost::system::errc::bad_file_descriptor) || (ec == boost::system::errc::operation_canceled)) {
            TAU_LOG_DEBUG("Acceptor stopped, error: " << ec.message());
            return;
        }
        TAU_LOG_ERROR("Failed accept from client, error: " << ec.message());
    } else {
        try{
            Connection::CreateAndStart(socket, _request_callback);
        } catch(const std::exception& ex) {
            TAU_LOG_WARNING("Connection failed, exception: " << ex.what());
        }
    }

    Accept();
}

}
