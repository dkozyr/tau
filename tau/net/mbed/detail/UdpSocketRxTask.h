#pragma once

#include <tau/memory/Buffer.h>
#include <tau/net/Endpoint.h>
#include <tau/common/StaticQueue.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace tau::net::detail {

struct PacketContext {
    Buffer packet;
    Endpoint endpoint;
};

using UdpSocketRxBuffer = StaticQueue<PacketContext, 64>;

struct UdpSocketRxContext {
    Allocator& allocator;
    UdpSocketRxBuffer& buffer;
    
    int socket = 0;
    TaskHandle_t task = nullptr;
    volatile bool stop = false;
};

void UdpSocketRxTask(void* arg);

}
