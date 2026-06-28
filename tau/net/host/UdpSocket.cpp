#include "tau/net/host/UdpSocket.h"
#include "tau/common/Log.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

namespace tau::net {

UdpSocket::UdpSocket(Options&& options)
    : _allocator(options.allocator)
    , _rx_task(detail::UdpSocketRxContext{
        .allocator = _allocator,
        .buffer = _buffer,
    })
{
    _socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if(_socket < 0) {
        TAU_LOG_ERROR("socket failed, error: " << _socket);
        return;
    }

    if(options.multicast_address) {
        int reuse = 1;
        auto error = setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));
        if(error < 0) {
            TAU_LOG_ERROR("setsockopt SO_REUSEADDR failed, error: " << error << ", errno: " << errno);
            close(_socket);
            return;
        }
    }

    sockaddr_in local_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(options.local_port.value_or(0)),
        .sin_addr = {
            .s_addr = options.multicast_address.has_value()
                    ? INADDR_ANY
                    : options.local_address.GetUint32(true)
        },
        .sin_zero = {}
    };
    auto error = bind(_socket, (sockaddr*)&local_addr, sizeof(local_addr));
    if(error < 0) {
        TAU_LOG_ERROR("bind failed, error: " << error << ", errno: " << errno);
        close(_socket);
        return;
    }

    if(options.multicast_address) {
        ip_mreq mreq;
        mreq.imr_multiaddr.s_addr = options.multicast_address->GetUint32(true); 
        mreq.imr_interface.s_addr = INADDR_ANY; 
        auto error = setsockopt(_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq));
        if(error < 0) {
            TAU_LOG_ERROR("setsockopt failed, error: " << error << ", errno: " << errno);
            close(_socket);
            return;
        }

        // char loop = 0;
        // setsockopt(_socket, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));

        // unsigned char ttl = 1;
        // setsockopt(_socket, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
    }

    SetRecvBufferSize(64 * 1024);
    UpdateLocalEndpoint();
    StartRxTask();
}

const std::optional<Endpoint>& UdpSocket::GetLocalEndpoint() const {
    return _local_endpoint;
}

UdpSocket::~UdpSocket() {
    _rx_task.stop = true;
    while(_rx_task.socket > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    _rx_thread->join();
}

void UdpSocket::SetRecvCallback(RecvCallback callback) {
    _recv_callback = std::move(callback);
}

void UdpSocket::Send(Buffer&& packet, const Endpoint& remote_endpoint) {
    Send(ToConst(packet.GetView()), remote_endpoint);
}

void UdpSocket::Send(const BufferViewConst& view, const Endpoint& remote_endpoint) {
    sockaddr_in dest = {
        .sin_family = AF_INET,
        .sin_port = htons(remote_endpoint.port),
        .sin_addr = {.s_addr = remote_endpoint.address.GetUint32(true)},
        .sin_zero = {}
    };

    auto error = sendto(_socket, view.ptr, view.size, 0, (sockaddr *)&dest, sizeof(dest));
    if(error < 0) {
        TAU_LOG_WARNING_THR(128, "sendto failed, error: " << error << ", view.size: " << view.size);
    }
}

bool UdpSocket::Receive() {
    size_t count = 0;
    while(!_buffer.Empty()) {
        detail::PacketContext item {
            .packet = Buffer::CreateEmpty(_allocator),
            .endpoint = Endpoint{
                .address = IpAddress{},
                .port = 0
            }
        };

        if(_buffer.TryPop(item)) {
            TAU_LOG_TRACE("size: " << item.packet.GetSize() << ", endpoint: " << item.endpoint);
            _recv_callback(std::move(item.packet), item.endpoint);
        } else {
            TAU_LOG_WARNING("TryPop failed");
            break;
        }
        count++;
    }
    return (count > 0);
}

void UdpSocket::StartRxTask() {
    _rx_task.socket = _socket;
    _rx_thread.emplace([this]() {
        UdpSocketRxTask(_rx_task);
    });
}

void UdpSocket::SetRecvBufferSize(int recv_buffer_size) {
    auto error = setsockopt(_socket, SOL_SOCKET, SO_RCVBUF, &recv_buffer_size, sizeof(recv_buffer_size));
    if(error < 0) {
        TAU_LOG_WARNING("setsockopt failed, error: " << error);
    }

    recv_buffer_size = 0;
    socklen_t len = sizeof(recv_buffer_size);
    error = getsockopt(_socket, SOL_SOCKET, SO_RCVBUF, &recv_buffer_size, &len);
    if(error < 0) {
        TAU_LOG_WARNING("getsockopt failed, error: " << error);
    } else {
        TAU_LOG_DEBUG("getsockopt actual recv_buffer_size: " << recv_buffer_size);
    }
}

void UdpSocket::UpdateLocalEndpoint() {
    sockaddr_storage storage;
    socklen_t size = sizeof(sockaddr_storage);
    auto error = getsockname(_socket, reinterpret_cast<sockaddr*>(&storage), &size);
    if(error < 0) {
        TAU_LOG_ERROR("getsockname failed, error: " << error);
        return;
    }

    if(storage.ss_family == AF_INET) {
        auto in = reinterpret_cast<sockaddr_in*>(&storage);
        _local_endpoint = Endpoint{
            .address = IpAddress{in->sin_addr.s_addr, true},
            .port = ntohs(in->sin_port)
        };
    }
}

}
