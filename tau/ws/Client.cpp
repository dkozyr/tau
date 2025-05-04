#include "tau/ws/Client.h"
#include "tau/common/Log.h"
#include <boost/algorithm/string/trim.hpp>

namespace tau::ws {

Client::Client(Options&& options, Executor executor, asio_ssl::context& ssl_ctx)
    : _options(std::move(options))
    , _executor(executor)
    , _resolver(executor)
    , _socket(asio::make_strand(executor), ssl_ctx) {
}

Client::~Client() {
    beast_ec ec;
    _socket.next_layer().lowest_layer().close(ec);
}

void Client::Start() {
    _resolver.async_resolve(_options.host, std::to_string(_options.port),
        [self = shared_from_this()](beast_ec ec, asio_tcp::resolver::results_type results) {
            self->OnResolve(ec, std::move(results));
        });
}

void Client::PostMessage(std::string message) {
    asio::post(_executor, [self = shared_from_this(), message = std::move(message)]() mutable {
        self->DoPostMessage(std::move(message));
    });
}

void Client::Close() {
    if(_closed) {
        return;
    }

    _closed = true;
    _socket.async_close(beast_ws::close_code::normal,
        [self = shared_from_this()](beast_ec ec) {
            self->OnClose(ec);
        });
}

void Client::OnResolve(beast_ec ec, asio_tcp::resolver::results_type results) {
    if(ec) {
        TAU_LOG_WARNING("Error: " << ec.message());
        return;
    }

    beast::get_lowest_layer(_socket).expires_after(kHandshakeTimeout);

    beast::get_lowest_layer(_socket).async_connect(results,
        [self = shared_from_this()](beast_ec ec, asio_tcp::resolver::results_type::endpoint_type endpoint) {
            self->OnConnect(ec, std::move(endpoint));
        });
}

void Client::OnConnect(beast_ec ec, asio_tcp::resolver::results_type::endpoint_type) {
    if(ec) {
        TAU_LOG_WARNING("Error: " << ec.message());
        return;
    }

    beast::get_lowest_layer(_socket).expires_after(kHandshakeTimeout);

    _socket.next_layer().async_handshake(asio_ssl::stream_base::client,
        [self = shared_from_this()](beast_ec ec) {
            self->OnSslHandshake(ec);
        });
}

void Client::OnSslHandshake(beast_ec ec) {
    if(ec) {
        TAU_LOG_WARNING("Error: " << ec.message());
        return;
    }

    beast::get_lowest_layer(_socket).expires_never();

    beast_ws::stream_base::timeout timeouts{
        .handshake_timeout = kHandshakeTimeout,
        .idle_timeout = kIdleTimeout,
        .keep_alive_pings = true
    };
    _socket.set_option(timeouts);

    _socket.set_option(beast_ws::stream_base::decorator(
        [](beast_ws::request_type& request) {
            request.set(beast_http::field::user_agent, std::string("ws-client-") + std::string(BOOST_BEAST_VERSION_STRING));
            request.set(beast_http::field::origin, std::string("origin-custom-value"));
        }));

    _socket.async_handshake(_response, _options.host, "/",
        [self = shared_from_this()](beast_ec ec) {
            self->OnHandshake(ec);
        });
}

void Client::OnHandshake(beast_ec ec) {
    if(ec) {
        TAU_LOG_WARNING("Error: " << ec.message());
        return;
    }

    _on_connected_callback();

    DoRead();
}

void Client::DoRead() {
    _buffer.clear();
    _socket.async_read(_buffer,
        [self = shared_from_this()](beast_ec ec, std::size_t bytes_transferred) {
            self->OnRead(ec, bytes_transferred);
        });
}

void Client::OnRead(beast_ec ec, std::size_t bytes_transferred) {
    if(ec) {
        if((ec != beast_ws::error::closed) && (ec != asio::error::eof)) {
            TAU_LOG_WARNING("Error: " << ec.message() << ", bytes_transferred: " << bytes_transferred);
        }
        return;
    }

    auto response = boost::algorithm::trim_copy(beast::buffers_to_string(_buffer.data()));
    _on_message_callback(std::move(response));

    DoRead();
}

void Client::DoPostMessage(std::string&& message) {
    _message_queue.push_back(std::move(message));
    if(_message_queue.size() == 1) {
        DoWriteLoop();
    }
}

void Client::DoWriteLoop() {
    if(_message_queue.empty()) {
        return;
    }

    _socket.async_write(
        asio::buffer(_message_queue.front()),
        [self = shared_from_this(), this] (beast_ec ec, size_t bytes_transferred) {
            if(ec) {
                if(ec != boost::system::errc::operation_canceled) {
                    TAU_LOG_WARNING("Error: " << ec.message() << ", bytes_transferred: " << bytes_transferred);
                }
                return;
            }

            _message_queue.pop_front();
            DoWriteLoop();
        });
}

void Client::OnClose(beast_ec ec) {
    if(ec) {
        TAU_LOG_WARNING("Error: " << ec.message());
    }
}

}
