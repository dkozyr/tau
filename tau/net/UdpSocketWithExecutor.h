#pragma once

#include "tau/memory/Buffer.h"
#include "tau/asio/Common.h"
#include "tau/net/Endpoint.h"

namespace tau::net {

class UdpSocketWithExecutor : public std::enable_shared_from_this<UdpSocketWithExecutor> {
public:
    struct Options {
        Allocator& allocator;
        Executor executor;
        IpAddress local_address;
        std::optional<uint16_t> local_port = std::nullopt;
        std::optional<IpAddress> multicast_address = {};
    };

    using RecvCallback = std::function<void(Buffer&& packet, Endpoint remote_endpoint)>;
    using ErrorCallback = std::function<void(boost_ec)>;

public:
    static auto Create(Options&& options) {
        std::shared_ptr<UdpSocketWithExecutor> self(new UdpSocketWithExecutor(std::move(options)));
        return self;
    }
    ~UdpSocketWithExecutor();

    void SetRecvCallback(RecvCallback callback);
    void SetErrorCallback(ErrorCallback callback) { _error_callback = std::move(callback); }

    void Send(Buffer&& packet, Endpoint remote_endpoint);
    void Send(const BufferViewConst& packet, Endpoint remote_endpoint);

    const std::optional<Endpoint>& GetLocalEndpoint() const { return _local_endpoint; }

private:
    UdpSocketWithExecutor(Options&& options);

    void ReceiveAvailable();
    void ReceiveAsync();
    void OnReceiveAsync(const boost_ec& ec, size_t bytes);

private:
    Allocator& _allocator;
    Executor _executor;
    asio::ip::udp::socket _socket;
    std::optional<Endpoint> _local_endpoint;

    struct Context {
        Buffer buffer;
        asio::ip::udp::endpoint remote_endpoint = {};
    };
    Context _ctx;

    RecvCallback _recv_callback;
    ErrorCallback _error_callback;
};

using UdpSocketWithExecutorPtr = std::shared_ptr<UdpSocketWithExecutor>;

}
