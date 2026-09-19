#include "tests/rtsp-test-lib/Server.h"
#include "tau/asio/ToString.h"
#include "tau/common/Log.h"

namespace tau::rtsp {

Server::Server(Callback callback)
    : _pool(1)
    , _acceptor(_pool.GetExecutor(), {asio::ip::make_address("127.0.0.1"), 0})
    , _socket(_pool.GetExecutor()), _callback(std::move(callback)) {
    _acceptor.async_accept(_socket,[this](boost_ec ec) {
        if(ec) {
            TAU_LOG_WARNING("Accept error: " << ec);
            return;
        }
        Read();
    });
}

Server::~Server() {
    asio::post(_pool.GetExecutor(), [this]() {
        boost_ec ec;
        _acceptor.close(ec);
        if(ec) {
            TAU_LOG_WARNING("Close acceptor error: " << ec);
        }

        _socket.close(ec);
        if(ec) {
            TAU_LOG_WARNING("Close socket error: " << ec);
        }
    });
    _pool.Join();
}

uint16_t Server::Port() const {
    return _acceptor.local_endpoint().port();
}

void Server::SendBuffer(etl::string_view data) {
    boost_ec ec;
    asio::write(_socket, asio::buffer(data.data(), data.size()), ec);
    if(ec) {
        TAU_LOG_WARNING("Error: " << ec);
    }
}

void Server::Disconnect() {
    boost_ec ec;
    _socket.close(ec);
    if(ec) {
        TAU_LOG_WARNING("Disconnect error: " << ec);
    }
}

void Server::Read() {
    _socket.async_read_some(asio::buffer(_input), [this](boost_ec ec, size_t size) {
        OnRead(ec, size);
    });
}

void Server::OnRead(boost_ec ec, size_t size) {
    if(ec) {
        TAU_LOG_WARNING("Error: " << ec);
        return;
    }

    const auto parsed = _reader.Push({_input.data(), size}, [this](StreamReader::Message&& message) {
        _callback(*this, std::move(message));
    });
    if(parsed) {
        if(_socket.is_open()) {
            Read();
        }
    } else {
        TAU_LOG_WARNING("Invalid stream framing");
        Disconnect();
    }
}

}
