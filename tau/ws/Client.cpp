#include "tau/ws/Client.h"
#include "tau/asio/ToString.h"
#include "tau/common/Exception.h"
#include "tau/common/StdString.h"
#include "tau/common/Log.h"

namespace tau::ws {

Client::Client(Executor executor, Options&& options)
    : _options(std::move(options))
    , _executor(asio::make_strand(executor))
    , _resolver(_executor)
    , _socket(_executor, _options.ssl_ctx) {
    // Set SNI Hostname (many hosts need this to handshake successfully)
    if(!SSL_set_tlsext_host_name(_socket.next_layer().native_handle(), _options.host.data())) {
        TAU_EXCEPTION(std::runtime_error, "SSL_set_tlsext_host_name failed");
    }
}

Client::~Client() {
    TAU_LOG_TRACE("OK");
}

void Client::Start() {
    _resolver.async_resolve(std::string{_options.host.data()}, std::to_string(_options.port),
        [self_weak = weak_from_this()](beast_ec ec, asio::ip::tcp::resolver::results_type results) {
            if(auto self = self_weak.lock()) {
                self->OnResolve(ec, std::move(results));
            }
        });
}

void Client::PostMessage(String message) {
    asio::post(_executor,
        [self_weak = weak_from_this(), message = std::move(message)]() mutable {
            if(auto self = self_weak.lock()) {
                self->DoPostMessage(std::move(message));
            }
    });
}

void Client::Close(bool graceful) {
    if(_closed) {
        return;
    }
    _closed = true;

    if(graceful) {
        _socket.async_close(beast_ws::close_code::normal,
            [self_weak = weak_from_this()](beast_ec ec) {
                if(auto self = self_weak.lock()) {
                    self->OnClose(ec);
                }
            });
    } else {
        beast_ec ec;
        _socket.next_layer().lowest_layer().close(ec);
    }
}

void Client::OnResolve(beast_ec ec, asio::ip::tcp::resolver::results_type results) {
    if(ec) {
        TAU_LOG_WARNING("Error: " << ec);
        return;
    }

    beast::get_lowest_layer(_socket).expires_after(kHandshakeTimeout);

    beast::get_lowest_layer(_socket).async_connect(results,
        [self_weak = weak_from_this()](beast_ec ec, asio::ip::tcp::resolver::results_type::endpoint_type endpoint) {
            if(auto self = self_weak.lock()) {
                self->OnConnect(ec, std::move(endpoint));
            }
        });
}

void Client::OnConnect(beast_ec ec, asio::ip::tcp::resolver::results_type::endpoint_type) {
    if(ec) {
        TAU_LOG_WARNING("Error: " << ec);
        return;
    }

    beast::get_lowest_layer(_socket).expires_after(kHandshakeTimeout);

    _socket.next_layer().async_handshake(asio_ssl::stream_base::client,
        [self_weak = weak_from_this()](beast_ec ec) {
            if(auto self = self_weak.lock()) {
                self->OnSslHandshake(ec);
            }
        });
}

void Client::OnSslHandshake(beast_ec ec) {
    if(ec) {
        TAU_LOG_WARNING("Error: " << ec);
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
        [this](beast_ws::request_type& request) {
            request.set(beast_http::field::user_agent, "tau-ws-client-" BOOST_BEAST_VERSION_STRING);
            for(auto& field : _options.http_fields) {
                std::visit(overloaded{
                    [&](beast_http::field name) { request.set(name, ToStdStringView(field.value)); },
                    [&](etl::string_view name)  { request.set(ToStdStringView(name), ToStdStringView(field.value)); }
                }, field.name);
            }
        }));

    _socket.next_layer().set_verify_mode(asio_ssl::verify_peer);

    _socket.async_handshake(_response, _options.host.data(), _options.path.data(),
        [self_weak = weak_from_this()](beast_ec ec) {
            if(auto self = self_weak.lock()) {
                self->OnHandshake(ec);
            }
        });
}

void Client::OnHandshake(beast_ec ec) {
    if(ec) {
        TAU_LOG_WARNING("Error: " << ec);
        return;
    }

    _on_connected_callback();

    DoRead();
}

void Client::DoRead() {
    _buffer.clear();
    _socket.async_read(_buffer,
        [self_weak = weak_from_this()](beast_ec ec, std::size_t bytes_transferred) {
            if(auto self = self_weak.lock()) {
                self->OnRead(ec, bytes_transferred);
            }
        });
}

void Client::OnRead(beast_ec ec, std::size_t bytes_transferred) {
    if(ec) {
        if((ec != beast_ws::error::closed) && (ec != asio::error::eof)) {
            TAU_LOG_WARNING("Error: " << ec << ", bytes_transferred: " << bytes_transferred);
        }
        _on_error_callback(ec);
        return;
    }

    String response;
    const auto& data = _buffer.data();
    for(auto it = asio::buffer_sequence_begin(data); it != asio::buffer_sequence_end(data); ++it) {
        response.append(static_cast<const char*>(it->data()), it->size());
    }
    _on_message_callback(std::move(response));

    DoRead();
}

void Client::DoPostMessage(String&& message) {
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
        [self_weak = weak_from_this()] (beast_ec ec, size_t bytes_transferred) {
            if(auto self = self_weak.lock()) {
                self->OnWrite(ec, bytes_transferred);
            }
        });
}

void Client::OnWrite(beast_ec ec, size_t bytes_transferred) {
    if(ec) {
        if(ec != boost::system::errc::operation_canceled) {
            TAU_LOG_WARNING("Error: " << ec << ", bytes_transferred: " << bytes_transferred);
        }
        _on_error_callback(ec);
        return;
    }

    _message_queue.pop_front();
    DoWriteLoop();
}

void Client::OnClose(beast_ec ec) {
    if(ec) {
        TAU_LOG_WARNING("Error: " << ec);
    } else {
        _socket.next_layer().lowest_layer().close(ec);
    }
}

}
