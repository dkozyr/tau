#include "tau/net/UdpSocketWithExecutor.h"

namespace tau::net {

asio::ip::udp::endpoint ToEndpoint(const Endpoint& endpoint);
Endpoint ToEndpoint(const asio::ip::udp::endpoint& endpoint);

UdpSocketWithExecutor::UdpSocketWithExecutor(Options&& options)
    : _allocator(options.allocator)
    , _executor(std::move(options.executor))
    , _socket(_executor)
    , _ctx(Context{.buffer = Buffer::Create(_allocator)}) {
    const auto port = options.local_port.value_or(0);
    if(!options.multicast_address) {
        auto local_endpoint = ToEndpoint({options.local_address, port});
        _socket.open(local_endpoint.protocol());
        _socket.bind(local_endpoint);
    } else {
        asio::ip::udp::endpoint local_endpoint{asio::ip::address_v4::any(), port};
        _socket.open(local_endpoint.protocol());
        _socket.set_option(asio::ip::udp::socket::reuse_address(true));
        _socket.bind(local_endpoint);
        _socket.set_option(asio::ip::multicast::join_group(asio::ip::address_v4{options.multicast_address->GetUint32()}));
    }
    _socket.non_blocking(true);

    _local_endpoint = ToEndpoint(_socket.local_endpoint());
}

UdpSocketWithExecutor::~UdpSocketWithExecutor() {
    boost_ec ec;
    _socket.close(ec);
}

void UdpSocketWithExecutor::SetRecvCallback(RecvCallback callback) {
    const bool start_receiving = !_recv_callback;
    _recv_callback = std::move(callback);

    if(start_receiving) {
        asio::post(_executor, [self = shared_from_this()]() {
            self->ReceiveAsync();
        });
    }
}

void UdpSocketWithExecutor::Send(Buffer&& packet, Endpoint remote_endpoint) {
    Send(ToConst(packet.GetView()), remote_endpoint);
}

void UdpSocketWithExecutor::Send(const BufferViewConst& view, Endpoint remote_endpoint) {
    boost_ec ec;
    _socket.send_to(asio::buffer(view.ptr, view.size), ToEndpoint(remote_endpoint), 0, ec);
    if(ec && _error_callback) {
        _error_callback(ec);
    }
}

void UdpSocketWithExecutor::ReceiveAvailable() {
    boost_ec ec;
    while(_socket.available(ec)) {
        if(!ec) {
            break;
        }
        auto view = _ctx.buffer.GetViewWithCapacity();
        auto bytes = _socket.receive_from(asio::buffer(view.ptr, view.size), _ctx.remote_endpoint);
        _ctx.buffer.SetSize(bytes);
        _recv_callback(std::move(_ctx.buffer), ToEndpoint(_ctx.remote_endpoint));
        _ctx.buffer = Buffer::Create(_allocator);
    }
}

void UdpSocketWithExecutor::ReceiveAsync() {
    ReceiveAvailable();
    auto view = _ctx.buffer.GetViewWithCapacity();
    _socket.async_receive_from(asio::buffer(view.ptr, view.size), _ctx.remote_endpoint,
        [weak_self = weak_from_this()](const boost_ec& ec, size_t bytes) mutable {
            if(auto self = weak_self.lock()) {
                self->OnReceiveAsync(ec, bytes);
            }
        }
    );
}

void UdpSocketWithExecutor::OnReceiveAsync(const boost_ec& ec, size_t bytes) {
    if(!ec) {
        _ctx.buffer.SetSize(bytes);
        _recv_callback(std::move(_ctx.buffer), ToEndpoint(_ctx.remote_endpoint));
        _ctx.buffer = Buffer::Create(_allocator);
        ReceiveAsync();
    } else {
        if(_error_callback) {
            _error_callback(ec);
        }
    }
}

asio::ip::udp::endpoint ToEndpoint(const Endpoint& endpoint) {
    return asio::ip::udp::endpoint{
        asio::ip::address_v4{endpoint.address.GetUint32()},
        endpoint.port
    };
}

Endpoint ToEndpoint(const asio::ip::udp::endpoint& endpoint) {
    return Endpoint{
        .address = IpAddress{endpoint.address().to_v4().to_uint()},
        .port = endpoint.port()
    };
}

}
