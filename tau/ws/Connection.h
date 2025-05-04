#pragma once

#include "tau/asio/Ssl.h"
#include <string>
#include <deque>
#include <variant>

namespace tau::ws {

class Connection : public std::enable_shared_from_this<Connection> {
public:
    struct CloseMessage{};
    struct DoNothingMessage{};
    using Message = std::variant<std::string, CloseMessage, DoNothingMessage>;

    using ProcessMessageCallback = std::function<Message(std::string&&)>;

public:
    Connection(asio_tcp::socket&& socket, SslContext& ssl_ctx);
    ~Connection();

    void SetProcessMessageCallback(ProcessMessageCallback callback) { _process_message_callback = std::move(callback); }

    void Start();
    void Close();
    void PostMessage(std::string message);

private:
    using SocketType = beast_ws::stream<beast::ssl_stream<beast::tcp_stream>>;

private:
    void OnStart();
    void OnHandshake(beast_ec ec);
    void OnFirstRequest(beast_ec ec, std::size_t bytes_transferred);
    void OnAccept(beast_ec ec);
    void DoRead();
    void OnRead(beast_ec ec, std::size_t bytes_transferred);
    void DoPostMessage(Message message);
    void DoWriteLoop();
    void OnWrite(beast_ec ec, std::size_t bytes_transferred);
    void OnClose(beast_ec ec);

    static std::string CreateLogContext(const SocketType& socket);

private:
    SocketType _socket;
    const std::string _log_ctx;
    beast::flat_buffer _buffer;
    std::deque<Message> _message_queue;
    bool _is_closed = false;

    ProcessMessageCallback _process_message_callback;

    beast_request _request;
};

using ConnectionPtr = std::shared_ptr<Connection>;
using ConnectionWeakPtr = std::weak_ptr<Connection>;

}
