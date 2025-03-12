#pragma once

#include "tau/net/UdpSocket.h"
#include <utility>

namespace net {

using UdpSocketsPair = std::pair<UdpSocketPtr, UdpSocketPtr>;

inline UdpSocketsPair CreateUdpSocketsPair(UdpSocket::Options&& options, size_t attempts_count = 4) {
    for(size_t i = 0; i < attempts_count; ++i) {
        try {
            options.local_address.port.reset();
            auto socket1 = UdpSocket::Create(UdpSocket::Options{options});
            const auto port1 = socket1->GetLocalEndpoint().port();

            const bool odd_port = (port1 % 2 == 1);
            options.local_address.port = port1 + (odd_port ? -1 : +1);
            auto socket2 = UdpSocket::Create(UdpSocket::Options{options});
            if(odd_port) {
                std::swap(socket1, socket2);
            }
            return {std::move(socket1), std::move(socket2)};
        } catch(const std::exception&) {
            // perhaps next attempt will be successful
        }
    }
    return {nullptr, nullptr};
}

}
