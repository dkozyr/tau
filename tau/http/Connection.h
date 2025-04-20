#pragma once

#include "tau/asio/Ssl.h"
#include "tau/asio/Timer.h"
#include "tau/common/Variant.h"
#include <boost/beast/core/flat_buffer.hpp>
#include <memory>
#include <optional>

namespace tau::http {

class Connection : public std::enable_shared_from_this<Connection> {
public:
    static constexpr auto kTimeoutDefault = std::chrono::seconds(10);

    using Socket = std::variant<SslSocketPtr, std::shared_ptr<asio_tcp::socket>>;

    using ResponseCallback = std::function<void(beast_response&&)>;
    using RequestCallback = std::function<void(const beast_request&, const ResponseCallback& callback)>;

public:
    template<typename... TArgs>
    static void CreateAndStart(TArgs&&... args) {
        std::shared_ptr<Connection> self(new Connection(std::forward<TArgs>(args)...));
        self->Start();
    }

    ~Connection();

private:
    Connection(Socket socket, RequestCallback request_callback);

    void Start();
    void OnHandshake(beast_ec ec);
    void Read();
    void OnRead(beast_ec ec, size_t bytes_transferred);
    void Write(beast_response&& response);
    void OnWrite(beast_ec ec, size_t bytes_transferred);
    void Shutdown();
    void OnShutdown(beast_ec ec);
    void OnTimeout(beast_ec ec);

    static std::string CreateLogContext(const Socket& socket);

private:
    Socket _socket;
    Timer _timeout;
    RequestCallback _request_callback;
    beast_request _request;
    std::optional<beast_response> _response;
    beast::flat_buffer _buffer;
    const std::string _log_ctx;
};

}
