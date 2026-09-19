#pragma once

#include "tau/rtsp/StreamReader.h"
#include "tau/asio/Common.h"
#include "tau/asio/ThreadPool.h"
#include <array>

namespace tau::rtsp {

class Server {
public:
    using Callback = std::function<void(Server&, StreamReader::Message&&)>;

public:
    explicit Server(Callback callback);
    ~Server();

    uint16_t Port() const;

    void SendBuffer(etl::string_view data);
    void Disconnect();

private:
    void Read();
    void OnRead(boost_ec ec, size_t size);

private:
    ThreadPool _pool;
    asio::ip::tcp::acceptor _acceptor;
    asio::ip::tcp::socket _socket;

    Callback _callback;

    StreamReader _reader;
    std::array<char, 4096> _input;
};

}
