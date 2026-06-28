#pragma once

#ifdef ESP_PLATFORM
    #include <tau/net/mbed/UdpSocket.h>
#else
    #include <tau/net/host/UdpSocket.h>
#endif
