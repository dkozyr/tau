#pragma once

#ifdef ESP_PLATFORM
    #include <tau/net/mbed/Interface.h>
#else
    #include <tau/net/host/Interface.h>
#endif
