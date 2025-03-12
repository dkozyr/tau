#pragma once

#include "tau/net/IpAddress.h"
#include "tau/memory/Buffer.h"
#include "tau/asio/Common.h"

namespace net {

class UdpSocket : public std::enable_shared_from_this<UdpSocket> {
public:
    struct Options {
        Allocator& allocator;
        Executor executor;
        IpAddress local_address;
    };

    using RecvCallback = std::function<void(Buffer&& packet, asio_udp::endpoint remote_endpoint)>;
    using ErrorCallback = std::function<void(boost_ec)>;

public:
    static auto Create(Options&& options) {
        std::shared_ptr<UdpSocket> self(new UdpSocket(std::move(options)));
        return self;
    }
    ~UdpSocket();

    void SetRecvCallback(RecvCallback callback);
    void SetErrorCallback(ErrorCallback callback) { _error_callback = std::move(callback); }

    void Send(Buffer&& packet, asio_udp::endpoint remote_endpoint);
    void Send(const BufferViewConst& packet, asio_udp::endpoint remote_endpoint);

    asio_udp::endpoint GetLocalEndpoint() const { return _socket.local_endpoint(); }

private:
    UdpSocket(Options&& options);

    void ReceiveAsync();
    void OnReceiveAsync(const boost_ec& ec, size_t bytes);

private:
    Allocator& _allocator;
    Executor _executor;
    asio_udp::socket _socket;

    struct Context {
        Buffer buffer;
        asio_udp::endpoint remote_endpoint = {};
    };
    Context _ctx;

    RecvCallback _recv_callback;
    ErrorCallback _error_callback;
};

using UdpSocketPtr = std::shared_ptr<UdpSocket>;

}
