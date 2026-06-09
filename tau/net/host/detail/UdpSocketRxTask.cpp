#include "UdpSocketRxTask.h"
#include <tau/common/Log.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>

namespace tau::net::detail {

void UdpSocketRxTask(UdpSocketRxContext& ctx) {
    TAU_LOG_DEBUG("Start");
    auto& buffer = ctx.buffer;
    auto& socket = ctx.socket;

    timeval tv = {
        .tv_sec = 0,
        .tv_usec = 200 * 1000 // 200 ms
    };
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    while(!ctx.stop) {
        PacketContext item{
            .packet = Buffer::Create(ctx.allocator),
            .endpoint = Endpoint{
                .address = IpAddress{},
                .port = 0
            }
        };

        sockaddr_storage src_addr;
        socklen_t src_addr_size = sizeof(src_addr);
        auto view = item.packet.GetViewWithCapacity();
        auto bytes = recvfrom(socket, view.ptr, view.size, 0, (sockaddr*)&src_addr, &src_addr_size);
        if(bytes > 0) {
            item.packet.SetSize(bytes);

            if(src_addr.ss_family == AF_INET) {
                auto in = reinterpret_cast<sockaddr_in*>(&src_addr);
                item.endpoint.address = MakeIpAddressV4(in->sin_addr.s_addr, true);
                item.endpoint.port = ntohs(in->sin_port);
            }

            if(buffer.Full() || !buffer.Push(std::move(item))) {
                TAU_LOG_WARNING("Push failed, full: " << buffer.Full());
            }
            continue;
        }
        if(bytes == 0) {
            continue;
        }

        if(ctx.stop) {
            break;
        }

        int error = bytes;
        if((error == EINTR) || (error == EAGAIN) || (error == EWOULDBLOCK)) {
            continue;
        }
        if((error == EBADF) || (error == ENOTSOCK)) {
            break;
        }
        TAU_LOG_WARNING("recvfrom error: " << error);
        break;
    }

    TAU_LOG_DEBUG("Close socket");
    if(socket >= 0) {
        close(socket);
        socket = -1;
    }

    TAU_LOG_DEBUG("Exit");
}

}
