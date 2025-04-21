#include "tau/http/Connection.h"
#include "tau/common/Log.h"
#include <sstream>

namespace tau::http {

constexpr auto kBufferLimit = 0x4000;

Connection::Connection(Socket socket, RequestCallback request_callback)
    : _socket(std::move(socket))
    , _timeout(
        std::visit([](auto socket) { return socket->get_executor(); }, _socket),
        kTimeoutDefault)
    , _request_callback(std::move(request_callback))
    , _buffer(kBufferLimit)
    , _log_ctx(CreateLogContext(_socket)) {
}

Connection::~Connection() {
    beast_ec ec;
    _timeout.cancel(ec);

    std::visit(overloaded{
        [&ec](SslSocketPtr& socket)                      { socket->lowest_layer().close(ec); },
        [&ec](std::shared_ptr<asio_tcp::socket>& socket) { socket->close(ec); }
    }, _socket);
}

void Connection::Start() {
    _timeout.async_wait(
        [self = this->shared_from_this()](beast_ec ec) {
            self->OnTimeout(ec);
        });

    std::visit(overloaded{
        [this](SslSocketPtr& socket) {
            socket->async_handshake(asio_ssl::stream_base::server,
                [self = this->shared_from_this()](boost_ec ec) {
                    self->OnHandshake(ec);
                }
            );
        },
        [this](std::shared_ptr<asio_tcp::socket>&) {
            Read();
        }
    }, _socket);
}

void Connection::OnHandshake(beast_ec ec) {
    if(ec) {
        TAU_LOG_DEBUG(_log_ctx << "error: " << ec.message());
        Shutdown();
        return;
    }
    Read();
}

void Connection::Read() {
    _request = {};
    std::visit(
        [this](auto& socket) {
            beast_http::async_read(
                *socket, _buffer, _request,
                [self = this->shared_from_this()](boost_ec ec, size_t bytes_transferred) {
                    self->OnRead(ec, bytes_transferred);
                }
            );
        }, _socket);
}

void Connection::OnRead(beast_ec ec, size_t bytes_transferred) {
    if(ec) {
        if(ec != beast_http::error::end_of_stream) {
            TAU_LOG_WARNING(_log_ctx << "Stopped: " << ec.message() << ", error: " << ec.value() << ", bytes_transferred: " << bytes_transferred);
        }
        Shutdown();
        return;
    }
    _request_callback(_request,
        [self = this->shared_from_this()](beast_response&& response) {
            self->Write(std::move(response));
        });
}

void Connection::Write(beast_response&& response) {
    _response.emplace(std::move(response));
    _response->prepare_payload();
    std::visit(
        [this](auto& socket) {
            beast_http::async_write(*socket, *_response,
                [self = this->shared_from_this()](beast_ec ec, size_t bytes_transferred) {
                    self->OnWrite(ec, bytes_transferred);
                });
        }, _socket);
}

void Connection::OnWrite(beast_ec ec, size_t bytes_transferred) {
    if(ec) {
        TAU_LOG_WARNING(_log_ctx << "Error: " << ec.message() << ", bytes_transferred: " << bytes_transferred);
    }

    Shutdown();
}

void Connection::Shutdown() {
    std::visit(overloaded{
        [this](SslSocketPtr& socket) {
            socket->async_shutdown(
                [self = this->shared_from_this()](beast_ec ec) {
                    self->OnShutdown(ec);
                });
        },
        [](std::shared_ptr<asio_tcp::socket>& socket) {
            beast_ec ec;
            socket->shutdown(asio_tcp::socket::shutdown_send, ec);
        }
    }, _socket);

    beast_ec ec;
    _timeout.cancel(ec);
}

void Connection::OnShutdown(beast_ec ec) {
    if(ec) {
        TAU_LOG_DEBUG(_log_ctx << "error: " << ec.message());
    }
}

void Connection::OnTimeout(beast_ec ec) {
    if(!ec) {
        std::visit(overloaded{
            [&ec](SslSocketPtr& socket)                      { socket->lowest_layer().close(ec); },
            [&ec](std::shared_ptr<asio_tcp::socket>& socket) { socket->close(ec); }
        }, _socket);

        _timeout.cancel(ec);
    }
}

std::string Connection::CreateLogContext(const Socket& socket) {
    std::stringstream ss;
    std::visit(overloaded{
        [&ss](const SslSocketPtr& socket) {
            ss << "[" << socket->lowest_layer().local_endpoint() << " -> " << socket->lowest_layer().remote_endpoint() << "] ";
        },
        [&ss](const std::shared_ptr<asio_tcp::socket>& socket) {
            ss << "[" << socket->local_endpoint() << " -> " << socket->remote_endpoint() << "] ";
        }
    }, socket);
    return ss.str();
}

}
