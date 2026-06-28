#pragma once

#ifdef ESP_PLATFORM
    #include <tau/dtls/mbed/Session.h>
#else
    #include <tau/dtls/host/Session.h>
#endif
