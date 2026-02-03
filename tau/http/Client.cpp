#include "tau/http/Client.h"
#include "tau/common/Exception.h"
#include "tau/common/Log.h"
#include <openssl/ssl.h>

namespace tau::http {

Client::Client(Executor executor, Options&& options, ResponseCallback&& callback)
    : _options(std::move(options))
    , _response_callback(std::move(callback))
    , _resolver(executor)
    , _socket(CreateSocket(executor, _options.ssl_ctx))
    , _timeout(executor, kTimeoutDefault)
{
    if(_options.ssl_ctx) {
        // Set SNI Hostname (many hosts need this to handshake successfully)
        auto& socket = std::get<SslSocket>(_socket);
        if(!SSL_set_tlsext_host_name(socket.native_handle(), _options.host.c_str())) {
            TAU_EXCEPTION(std::runtime_error, "SSL_set_tlsext_host_name failed");
        }
    }
}

void Client::Start() {
    _timeout.async_wait(
        [self = this->shared_from_this()](beast_ec ec) {
            self->OnTimeout(ec);
        });

    _resolver.async_resolve(_options.host, std::to_string(_options.port),
        [self = shared_from_this()](boost_ec ec, asio_tcp::resolver::results_type results) {
            self->OnResolve(ec, std::move(results));
        });
}

void Client::OnResolve(boost_ec ec, asio_tcp::resolver::results_type results) {
    if(ec) {
        TAU_LOG_WARNING("ec: " << ec << ", " << ec.message());
        _timeout.cancel();
        return;
    }

    std::visit(overloaded{
        [this, &results](SslSocket& socket) {
            asio::async_connect(socket.lowest_layer(), results.begin(), results.end(),
            [self = shared_from_this()](boost_ec ec, asio_tcp::resolver::results_type::iterator) mutable {
                self->OnConnect(ec);
            });
        },
        [this, &results](asio_tcp::socket& socket) {
            asio::async_connect(socket.lowest_layer(), results.begin(), results.end(),
            [self = shared_from_this()](boost_ec ec, asio_tcp::resolver::results_type::iterator) mutable {
                self->OnHandshake(ec);
            });
        }
    }, _socket);
}

void Client::OnConnect(boost_ec ec) {
    if(ec) {
        TAU_LOG_WARNING("ec: " << ec << ", " << ec.message());
        _timeout.cancel();
        return;
    }

    auto& socket = std::get<SslSocket>(_socket);
    socket.async_handshake(asio_ssl::stream_base::client,
        [self = shared_from_this()](boost_ec ec) {
            self->OnHandshake(ec);
        }
    );
}

void Client::OnHandshake(boost_ec ec) {
    if(ec) {
        TAU_LOG_WARNING("ec: " << ec << ", " << ec.message());
        Shutdown();
        return;
    }

    _request.version(11); // 1.1 version => 11
    _request.method(_options.method);
    _request.target(_options.target);
    _request.set(beast_http::field::host, _options.host + ":" + std::to_string(_options.port));
    for(auto& field : _options.fields) {
        std::visit([&](const auto& key) { _request.set(key, field.value); }, field.type);
    }

    // We specify the "Connection: close" header so that the server will close the socket after transmitting the response
    _request.set(beast_http::field::connection, "close");

    beast::ostream(_request.body()) << _options.body;
    _request.prepare_payload();
    TAU_LOG_TRACE("Request: " << _request);

    //TODO: revise it:
    // beast::get_lowest_layer(_socket).expires_after(timeout);

    std::visit([this](auto& socket) {
        beast_http::async_write(socket, _request,
            [self = shared_from_this()](boost_ec ec, size_t bytes) mutable {
                self->OnWrite(ec, bytes);
            });
    }, _socket);
}

void Client::OnWrite(boost_ec ec, size_t bytes) {
    if(ec) {
        TAU_LOG_WARNING("ec: " << ec << ", " << ec.message() << ", bytes: " << bytes);
        Shutdown();
        return;
    }

    std::visit([this](auto& socket) {
        beast_http::async_read(socket, _buffer, _response,
            [self = shared_from_this()](boost_ec ec, size_t bytes) mutable {
                self->OnRead(ec, bytes);
            });
    }, _socket);
}

void Client::OnRead(boost_ec ec, size_t bytes) {
    _response_callback(ec, _response);

    if(ec) {
        TAU_LOG_WARNING("ec: " << ec << ", " << ec.message() << ", bytes: " << bytes);
    }

    Shutdown();
}

void Client::Shutdown() {
    boost_ec ec;
    _timeout.cancel();

    std::visit(overloaded{
        [this, &ec](SslSocket& socket) {
            socket.async_shutdown(
                [self = shared_from_this()](boost_ec ec) mutable {
                    self->OnShutdown(ec);
                });
        },
        [this, &ec](asio_tcp::socket& socket) {
            socket.shutdown(asio_tcp::socket::shutdown_both, ec);
            OnShutdown(ec);
        }
    }, _socket);
}

void Client::OnShutdown(beast_ec ec) {
    if(ec) {
        TAU_LOG_DEBUG("ec: " << ec << ", " << ec.message());
    }
}

void Client::OnTimeout(beast_ec ec) {
    if(!ec) {
        std::visit(overloaded{
            [&ec](SslSocket& socket)        { socket.lowest_layer().close(ec); },
            [&ec](asio_tcp::socket& socket) { socket.close(ec); }
        }, _socket);

        _timeout.cancel();
    }
}

Client::Socket Client::CreateSocket(Executor executor, const SslContextPtr& ssl_ctx) {
    if(ssl_ctx) {
        return SslSocket(executor, *ssl_ctx);
    } else {
        return asio_tcp::socket(executor);
    }
}

}
