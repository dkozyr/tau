#include "tau/ws/Connection.h"
#include "tau/common/Variant.h"
#include "tau/common/Log.h"
#include <boost/algorithm/string/trim.hpp>

namespace tau::ws {

Connection::Connection(asio_tcp::socket&& socket, SslContext& ssl_ctx)
    : _socket(std::move(socket), ssl_ctx)
    , _log_ctx(CreateLogContext(_socket)) {
    TAU_LOG_DEBUG(_log_ctx);
}

Connection::~Connection() {
    TAU_LOG_DEBUG(_log_ctx);
    beast_ec ec;
    _socket.next_layer().lowest_layer().close(ec);
}

void Connection::Start() {
    asio::dispatch(_socket.get_executor(),
        [self = this->shared_from_this()]() {
            self->OnStart();
        });
}

void Connection::Close() {
    _is_closed = true;
    asio::post(_socket.get_executor(),
        [self = shared_from_this()] {
            self->DoPostMessage(CloseMessage{});
        });
}

void Connection::PostMessage(std::string message) {
    if(_is_closed) {
        return;
    }
    asio::post(_socket.get_executor(),
        [self = shared_from_this(), message = std::move(message)] {
            self->DoPostMessage(std::move(message));
        });
}

void Connection::OnStart() {
    beast::get_lowest_layer(_socket).expires_after(std::chrono::seconds(30));
    _socket.next_layer().async_handshake(asio_ssl::stream_base::server,
        [self = shared_from_this()](beast_ec ec) {
            self->OnHandshake(ec);
        });
}

void Connection::OnHandshake(beast_ec ec) {
    if(ec) {
        TAU_LOG_WARNING("Error: " << ec.message());
        return;
    }

    beast::get_lowest_layer(_socket).expires_never();

    _socket.set_option(beast_ws::stream_base::timeout::suggested(beast::role_type::server));
    _socket.set_option(beast_ws::stream_base::decorator(
        [this](beast_ws::response_type& response) {
            response.set(beast_http::field::user_agent, std::string("tau-ws-server-") + std::string(BOOST_BEAST_VERSION_STRING));
            _process_response_callback(response);
        }));

    beast_http::async_read(_socket.next_layer(), _buffer, _request,
        [self = shared_from_this()](beast_ec ec, std::size_t bytes_transferred) {
            self->OnFirstRequest(ec, bytes_transferred);
        });
}

void Connection::OnFirstRequest(beast_ec ec, std::size_t bytes_transferred) {
    if(ec) {
        TAU_LOG_WARNING(_log_ctx<< "Error: " << ec.message() << ", bytes_transferred: " << bytes_transferred);
        return;
    }

    if(_validate_request_callback && !_validate_request_callback(_request)) {
        return;
    }

    _socket.async_accept(_request,
        [self = shared_from_this()](beast_ec ec) {
            self->OnAccept(ec);
        });
}

void Connection::OnAccept(beast_ec ec) {
    if(ec) {
        TAU_LOG_WARNING(_log_ctx << "Error: " << ec.message());
    } else {
        DoRead();
    }
}

void Connection::DoRead() {
    _buffer.clear();
    _socket.async_read(_buffer,
        [self = shared_from_this()](beast_ec ec, std::size_t bytes_transferred) {
            self->OnRead(ec, bytes_transferred);
        });
}

void Connection::OnRead(beast_ec ec, std::size_t bytes_transferred) {
    if(ec) {
        if((ec != beast_ws::error::closed) && (ec != asio::error::eof) && (ec != boost::system::errc::operation_canceled)) {
            TAU_LOG_WARNING(_log_ctx << "Error: " << ec.message() << ", bytes_transferred: " << bytes_transferred);
        }
        return;
    }

    auto request = boost::algorithm::trim_copy(beast::buffers_to_string(_buffer.data()));
    DoPostMessage(_process_message_callback(std::move(request)));

    DoRead();
}

void Connection::DoPostMessage(Message message) {
    _message_queue.push_back(std::move(message));
    if(_message_queue.size() == 1) {
        DoWriteLoop();
    }
}

void Connection::DoWriteLoop() {
    if(_message_queue.empty()) {
        return;
    }

    auto& message = _message_queue.front();
    std::visit(overloaded{
        [this](std::string& msg) {
            _socket.async_write(asio::buffer(msg),
                [self = shared_from_this()](beast_ec ec, std::size_t bytes_transferred) {
                    self->OnWrite(ec, bytes_transferred);
                });
        },
        [this](CloseMessage) {
            _socket.async_close(beast_ws::close_code::normal,
                [self = shared_from_this()](beast_ec ec) {
                    self->OnClose(ec);
                });
        },
        [this](DoNothingMessage) {
            _message_queue.pop_front();
            DoWriteLoop();
        }
    }, message);
}

void Connection::OnWrite(beast_ec ec, std::size_t bytes_transferred) {
    if(ec) {
        TAU_LOG_WARNING(_log_ctx << "Error: " << ec.message() << ", bytes_transferred: " << bytes_transferred);
        return;
    }

    _message_queue.pop_front();
    DoWriteLoop();
}

void Connection::OnClose(beast_ec ec) {
    if(ec) {
        if(ec != asio_ssl::error::stream_truncated) {
            TAU_LOG_WARNING(_log_ctx << "Error: " << ec.message());
        } else {
            TAU_LOG_DEBUG(_log_ctx << "Error: " << ec.message());
        }
    }
    _message_queue.pop_front();
}

std::string Connection::CreateLogContext(const SocketType& socket) {
    std::stringstream ss;
    ss << "[" << beast::get_lowest_layer(socket).socket().remote_endpoint() << "] ";
    return ss.str();
}

}
