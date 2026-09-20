#include "tau/rtsp/ConnectionState.h"
#include "tau/rtsp/ResponseReader.h"
#include "tau/common/String.h"
#include <stdexcept>

namespace tau::rtsp {

ConnectionState::Pending::Pending(Executor executor)
    : timer(executor)
{}

ConnectionState::ConnectionState(Executor executor, Connection::Options options)
    : _options(std::move(options))
    , _strand(asio::make_strand(executor))
    , _resolver(_strand)
    , _socket(_strand)
    , _write_timer(_strand)
{}

void ConnectionState::SetPacketCallback(Connection::PacketCallback callback) {
    asio::post(_strand, [state = shared_from_this(), callback = std::move(callback)]() mutable {
        if(!state->_closed) {
            state->_packet_callback = std::move(callback);
        }
    });
}

void ConnectionState::Connect() {
    if(_connecting || _connected || _closed) {
        return;
    }
    _connecting = true;

    const auto port = ToString<8>(_options.port);
    _resolver.async_resolve(_options.host.c_str(), port.data(),
        [self = shared_from_this()](boost_ec ec, asio::ip::tcp::resolver::results_type endpoints) {
            self->OnResolve(ec, std::move(endpoints));
        });
}

void ConnectionState::OnResolve(boost_ec ec, asio::ip::tcp::resolver::results_type endpoints) {
    if(_closed) {
        return;
    }
    if(ec) {
        Fail("RTSP resolve", ec);
        return;
    }

    asio::async_connect(_socket, endpoints,
        [self = shared_from_this()](boost_ec ec, const asio::ip::tcp::endpoint&) {
            self->OnConnect(ec);
        });
}

void ConnectionState::OnConnect(boost_ec ec) {
    if(_closed) {
        return;
    }
    if(ec) {
        Fail("RTSP connect", ec);
        return;
    }
    _connected = true;

    Read();
    WriteNext();
}

void ConnectionState::Read() {
    _socket.async_read_some(asio::buffer(_input),
        [self = shared_from_this()](boost_ec ec, size_t size) {
            self->OnRead(ec, size);
        });
}

void ConnectionState::OnRead(boost_ec ec, size_t size) {
    if(_closed) {
        return;
    }
    if(size != 0) {
        try {
            if(!_reader.Push({_input.data(), size}, [this](StreamReader::Message&& message) {
                OnMessage(std::move(message));
            })) {
                Fail("Invalid RTSP stream framing");
            }
        } catch(const std::exception& exception) {
            Fail(exception.what());
        }
    }
    if(_closed) {
        return;
    }
    if(ec) {
        Fail(_reader.Finish() ? "RTSP read" : "Truncated RTSP stream", ec);
        return;
    }

    Read();
}

void ConnectionState::OnMessage(StreamReader::Message&& message) {
    if(_closed) {
        return;
    }
    if(message.channel) {
        const auto callback = _packet_callback;
        if(callback) {
            callback(std::move(message));
        }
        return;
    }

    const auto response = ResponseReader::Read({message.data.data(), message.data.size()});
    if(!response) {
        Fail("Invalid RTSP response");
        return;
    }

    const auto cseq = GetHeaderValue(HeaderName::kCSeq, response->headers);
    if(cseq.empty() || (cseq.size() > CSeq::MAX_SIZE)) {
        Fail("Invalid RTSP response CSeq");
        return;
    }

    const auto it = _pending.find(CSeq{cseq.data(), cseq.size()});
    if(it == _pending.end()) {
        Fail("Unexpected RTSP response CSeq");
        return;
    }

    if((response->status_code >= 100) && (response->status_code < 200)) {
        return;
    }

    it->second->timer.cancel();
    it->second->result.set_value(std::move(message));
    _pending.erase(it);
}

void ConnectionState::WriteNext() {
    if(_closed || !_connected || _writing || _writes.empty()) {
        return;
    }
    _writing = true;

    auto write = _writes.front();
    _write_timer.expires_after(_options.timeout);
    _write_timer.async_wait([self = shared_from_this(), write](boost_ec ec) {
        if(!ec && !self->_writes.empty() && (self->_writes.front() == write)) {
            self->Fail("RTSP write timeout");
        }
    });

    asio::async_write(_socket, asio::buffer(write->data.data(), write->data.size()),
        [self = shared_from_this(), write](boost_ec ec, size_t) {
            self->OnWrite(ec);
        });
}

void ConnectionState::OnWrite(boost_ec ec) {
    if(_closed) {
        return;
    }
    _write_timer.cancel();
    if(ec) {
        Fail("RTSP write", ec);
        return;
    }

    _writes.front()->result.set_value();
    _writes.pop_front();
    _writing = false;
    WriteNext();
}

std::future<StreamReader::Message> ConnectionState::SendRequest(StreamReader::String request, etl::string_view cseq) {
    auto pending = std::make_shared<ConnectionState::Pending>(_strand);
    auto result = pending->result.get_future();
    if(cseq.empty() || (cseq.size() > CSeq::MAX_SIZE)) {
        pending->result.set_exception(std::make_exception_ptr(std::invalid_argument("Invalid RTSP request CSeq")));
        return result;
    }

    auto write = std::make_shared<ConnectionState::Write>();
    write->data.assign(request.data(), request.size());
    asio::post(_strand, [state = shared_from_this(), pending, write, cseq = CSeq{cseq.data(), cseq.size()}]() {
        if(state->_closed) {
            pending->result.set_exception(state->_failure);
            return;
        }
        if(cseq.empty() || state->_pending.count(cseq) || (state->_pending.size() >= ConnectionState::kQueueLimit) || (state->_writes.size() >= ConnectionState::kQueueLimit)) {
            pending->result.set_exception(std::make_exception_ptr(std::runtime_error("Invalid or excessive RTSP request")));
            return;
        }

        state->_pending.insert({cseq, pending});
        pending->timer.expires_after(state->_options.timeout);
        pending->timer.async_wait([state, cseq, pending](boost_ec ec) {
            const auto it = state->_pending.find(cseq);
            if(!ec && (it != state->_pending.end()) && (it->second == pending)) {
                state->Fail("RTSP request timeout");
            }
        });

        state->_writes.push_back(write);
        state->Connect();
        state->WriteNext();
    });
    return result;
}

std::future<void> ConnectionState::SendPacket(uint8_t channel, StreamReader::String packet) {
    auto write = std::make_shared<ConnectionState::Write>();
    auto result = write->result.get_future();
    if(packet.empty() || (packet.size() > 65535)) {
        write->result.set_exception(std::make_exception_ptr(std::invalid_argument("Invalid interleaved packet size")));
        return result;
    }

    write->data.push_back('$');
    write->data.push_back(static_cast<char>(channel));
    write->data.push_back(static_cast<char>(packet.size() / 256));
    write->data.push_back(static_cast<char>(packet.size() % 256));
    write->data.append(packet.data(), packet.size());
    asio::post(_strand, [state = shared_from_this(), write]() {
        if(state->_closed) {
            write->result.set_exception(state->_failure);
            return;
        }
        if(!state->_connected || (state->_writes.size() >= ConnectionState::kQueueLimit)) {
            write->result.set_exception(std::make_exception_ptr(std::runtime_error("RTSP connection unavailable or queue full")));
            state->Fail("RTSP packet send unavailable or queue full");
            return;
        }
        state->_writes.push_back(write);
        state->WriteNext();
    });
    return result;
}

void ConnectionState::SetCloseCallback(std::function<void()> callback) {
    asio::post(_strand, [state = shared_from_this(), callback = std::move(callback)]() mutable {
        if(state->_closed) {
            if(callback) {
                callback();
            }
        } else {
            state->_close_callback = std::move(callback);
        }
    });
}

void ConnectionState::AsyncClose(std::function<void()> completion) {
    asio::post(_strand, [state = shared_from_this(), completion = std::move(completion)]() mutable {
        state->Fail("RTSP connection closed");
        state->_packet_callback = {};
        state->_close_callback = {};
        if(completion) { completion(); }
    });
}

void ConnectionState::Close() {
    asio::dispatch(_strand, [state = shared_from_this()]() {
        state->Fail("RTSP connection closed");
    });
}

void ConnectionState::Fail(etl::string_view reason, boost_ec ec) {
    if(_closed) {
        return;
    }
    _closed = true;

    etl::string<256> message;
    message.assign(reason.data(), std::min<size_t>(reason.size(), 220));
    if(ec) {
        message.append(": ");
        message.append(ToString<16>(ec.value()));
    }
    _failure = std::make_exception_ptr(std::runtime_error(message.c_str()));

    _resolver.cancel();

    boost_ec close_ec;
    _socket.close(close_ec);
    _write_timer.cancel();

    for(auto& entry : _pending) {
        entry.second->timer.cancel();
        entry.second->result.set_exception(_failure);
    }
    _pending.clear();

    for(auto& write : _writes) {
        write->result.set_exception(_failure);
    }
    _writes.clear();

    _packet_callback = {};
    auto callback = std::move(_close_callback);
    _close_callback = {};
    if(callback) {
        callback();
    }
}

}
