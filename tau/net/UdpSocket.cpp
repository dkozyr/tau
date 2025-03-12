#include "tau/net/UdpSocket.h"

namespace net {

UdpSocket::UdpSocket(Options&& options)
    : _allocator(options.allocator)
    , _executor(std::move(options.executor))
    , _socket(_executor, {asio_ip::make_address(options.local_address.address), options.local_address.port.value_or(0)})
    , _ctx(Context{.buffer = Buffer::Create(_allocator)}) {
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

void UdpSocket::ReceiveAsync() {
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
