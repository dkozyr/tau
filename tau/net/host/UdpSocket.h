#pragma once

#include "detail/UdpSocketRxTask.h"
#include <memory>
#include <optional>
#include <functional>
#include <thread>

namespace tau::net {

class UdpSocket : public std::enable_shared_from_this<UdpSocket> {
public:
    struct Options {
        Allocator& allocator;
        IpAddress local_address;
        std::optional<uint16_t> local_port = std::nullopt;
        // std::optional<IpAddress> multicast_address = std::nullopt; //TODO: implement
    };

    using RecvCallback = std::function<void(Buffer&& packet, Endpoint remote_endpoint)>;
    // using ErrorCallback = std::function<void(boost_ec)>;

public:
    static auto Create(Options&& options) {
        std::shared_ptr<UdpSocket> self(new UdpSocket(std::move(options)));
        return self;
    }
    ~UdpSocket();

    void SetRecvCallback(RecvCallback callback);

    void Send(Buffer&& packet, const Endpoint& remote_endpoint);
    void Send(const BufferViewConst& packet, const Endpoint& remote_endpoint);

    bool Receive();

    const std::optional<Endpoint>& GetLocalEndpoint() const;

private:
    UdpSocket(Options&& options);

    void StartRxTask();
    void SetRecvBufferSize(int recv_buffer_size);
    void UpdateLocalEndpoint();

private:
    Allocator& _allocator;
    int _socket = 0;
    std::optional<Endpoint> _local_endpoint;

    detail::UdpSocketRxBuffer _buffer;
    detail::UdpSocketRxContext _rx_task;
    std::optional<std::thread> _rx_thread;

    RecvCallback _recv_callback;
};

using UdpSocketPtr = std::shared_ptr<UdpSocket>;

}
