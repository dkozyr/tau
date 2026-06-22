#include "tau/ws/Server.h"
#include "tau/asio/ToString.h"
#include "tau/common/Log.h"

namespace tau::ws {

Server::Server(Dependencies&& deps, Options&& options)
    : _executor(asio::make_strand(deps.executor))
    , _options(std::move(options))
    , _acceptor(_executor) {
    asio::ip::tcp::endpoint endpoint{asio::ip::make_address(_options.host.data()), _options.port};
    _acceptor.open(endpoint.protocol());
    _acceptor.set_option(Acceptor::reuse_address(true));
    _acceptor.bind(endpoint);
    _acceptor.listen(Acceptor::max_listen_connections);
}

Server::~Server() {
    boost_ec ec;
    _acceptor.close(ec);
    CloseConnections();
}

void Server::Start() {
    TAU_LOG_INFO("Listening: " << _acceptor.local_endpoint());
    DoAccept();
}

void Server::DoAccept() {
    _acceptor.async_accept(
        [this](beast_ec ec, Socket socket) {
            OnAccept(ec, std::move(socket));
        });
}

void Server::OnAccept(beast_ec ec, Socket socket) {
    if(ec) {
        if((ec != boost::system::errc::operation_canceled)) {
            TAU_LOG_WARNING("Error: " << ec);
        }
    } else {
        try {
            auto connection = std::make_shared<Connection>(std::move(socket), _options.ssl_ctx);
            connection->SetValidateRequest([this](const beast_request& request) {
                return ValidateRequest(request);
            });
            connection->SetProcessResponseCallback([this](beast_ws::response_type& response) {
                ProcessResponse(response);
            });
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

bool Server::ValidateRequest(const beast_request& request) const {
    if(_validate_request_callback) {
        return _validate_request_callback(request);
    }
    return true;
}

void Server::ProcessResponse(beast_ws::response_type& response) const {
    for(auto& field : _options.http_fields) {
        std::visit(overloaded{
            [&](beast_http::field name) { response.set(name, field.value.data()); },
            [&](etl::string_view name)  { response.set(name.data(), field.value.data()); }
        }, field.name);
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
