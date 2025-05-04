#include "tau/ws/Server.h"
#include "tau/common/Log.h"

namespace tau::ws {

Server::Server(Options&& options, Executor executor, SslContext& ssl_ctx)
    : _executor(asio::make_strand(executor))
    , _ssl_ctx(ssl_ctx)
    , _acceptor(_executor) {
    asio_tcp::endpoint endpoint{asio_ip::make_address(options.host), options.port};
    _acceptor.open(endpoint.protocol());
    _acceptor.set_option(asio_tcp::acceptor::reuse_address(true));
    _acceptor.bind(endpoint);
    _acceptor.listen(asio_tcp::acceptor::max_listen_connections);
}

Server::~Server() {
    CloseConnections();
    boost_ec ec;
    _acceptor.close(ec);
}

void Server::Start() {
    TAU_LOG_INFO("Listening: " << _acceptor.local_endpoint());
    DoAccept();
}

void Server::DoAccept() {
    _acceptor.async_accept(
        [this](beast_ec ec, asio_tcp::socket socket) {
            OnAccept(ec, std::move(socket));
        });
}

void Server::OnAccept(beast_ec ec, asio_tcp::socket socket) {
    if(ec) {
        if((ec != boost::system::errc::operation_canceled)) {
            TAU_LOG_WARNING("Error: " << ec.message());
        }
    } else {
        try {
            auto connection = std::make_shared<Connection>(std::move(socket), _ssl_ctx);
            _on_new_connection_callback(connection);
            connection->Start();

            _connections.push_back(ConnectionWeakPtr{connection});
        } catch(const std::exception& e) {
            TAU_LOG_WARNING("Exception: " << e.what());
        }

        ClearConnections();
        DoAccept();
    }
}

void Server::CloseConnections() {
    for(auto& connection_ptr : _connections) {
        if(auto connection = connection_ptr.lock()) {
            connection->Close();
        }
    }
}

void Server::ClearConnections() {
    _connections.remove_if(std::mem_fn(&ConnectionWeakPtr::expired));
}

}
