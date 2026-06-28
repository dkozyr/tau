#pragma once

#ifdef ESP_PLATFORM
    #include <tau/crypto/mbed/Certificate.h>
#else
    #include <tau/crypto/host/Certificate.h>
#endif
