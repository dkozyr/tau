#pragma once

#include "tau/asio/Common.h"
#include "tau/rtsp/StreamReader.h"
#include "tau/net/Uri.h"
#include <chrono>
#include <future>
#include <etl/string.h>

namespace tau::rtsp {

class ConnectionState;

class Connection {
public:
    struct Options {
        net::Uri::Host host;
        uint16_t port = 554;
        std::chrono::milliseconds timeout{5000};
    };

    using PacketCallback = StreamReader::Callback;

public:
    Connection(Executor executor, Options options);
    ~Connection();

    std::future<StreamReader::Message> SendRequest(StreamReader::String request, etl::string_view cseq);
    std::future<void> SendPacket(uint8_t channel, StreamReader::String packet);

    void SetPacketCallback(PacketCallback callback); // Callback runs on strand; no blocking waits
    void SetCloseCallback(std::function<void()> callback);

    // Completion on strand; user callbacks detached, internal I/O may still drain
    void AsyncClose(std::function<void()> completion);
    void Close(); // Cancel on strand; keep executor running through shutdown

private:
    std::shared_ptr<ConnectionState> _state;
};

}
