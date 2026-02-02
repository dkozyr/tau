#include "tau/net/UdpSocket.h"

namespace tau::net {

UdpSocket::UdpSocket(Options&& options)
    : _allocator(options.allocator)
    , _executor(std::move(options.executor))
    , _socket(_executor)
    , _ctx(Context{.buffer = Buffer::Create(_allocator)}) {
    if(options.multicast_address.empty()) {
        Endpoint local_endpoint{asio_ip::make_address(options.local_address), options.local_port.value_or(0)};
        _socket.open(local_endpoint.protocol());
        _socket.bind(local_endpoint);
    } else {
        Endpoint local_endpoint(IpAddressV4::any(), options.local_port.value_or(0));
        _socket.open(local_endpoint.protocol());
        _socket.set_option(asio_udp::socket::reuse_address(true));
        _socket.bind(local_endpoint);
        _socket.set_option(asio_ip::multicast::join_group(asio_ip::make_address(options.multicast_address)));
    }
    _socket.non_blocking(true);
}

UdpSocket::~UdpSocket() {
    boost_ec ec;
    _socket.close(ec);
}

void UdpSocket::SetRecvCallback(RecvCallback callback) {
    const bool start_receiving = !_recv_callback;
    _recv_callback = std::move(callback);

    if(start_receiving) {
        asio::post(_executor, [self = shared_from_this()]() {
            self->ReceiveAsync();
        });
    }
}

void UdpSocket::Send(Buffer&& packet, asio_udp::endpoint remote_endpoint) {
    Send(ToConst(packet.GetView()), remote_endpoint);
}

void UdpSocket::Send(const BufferViewConst& view, asio_udp::endpoint remote_endpoint) {
    boost_ec ec;
    _socket.send_to(asio::buffer(view.ptr, view.size), remote_endpoint, 0, ec);
    if(ec && _error_callback) {
        _error_callback(ec);
    }
}

void UdpSocket::ReceiveAvailable() {
    boost_ec ec;
    while(_socket.available(ec)) {
        if(!ec) {
            break;
        }
        auto view = _ctx.buffer.GetViewWithCapacity();
        auto bytes = _socket.receive_from(asio::buffer(view.ptr, view.size), _ctx.remote_endpoint);
        _ctx.buffer.SetSize(bytes);
        _recv_callback(std::move(_ctx.buffer), _ctx.remote_endpoint);
        _ctx.buffer = Buffer::Create(_allocator);
    }
}

void UdpSocket::ReceiveAsync() {
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

void UdpSocket::OnReceiveAsync(const boost_ec& ec, size_t bytes) {
    if(!ec) {
        _ctx.buffer.SetSize(bytes);
        _recv_callback(std::move(_ctx.buffer), _ctx.remote_endpoint);
        _ctx.buffer = Buffer::Create(_allocator);
        ReceiveAsync();
    } else {
        if(_error_callback) {
            _error_callback(ec);
        }
    }
}

}
