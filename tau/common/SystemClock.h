#pragma once

#ifdef ESP_PLATFORM
    #include <tau/common/mbed/SystemClock.h>
#else
    #include <tau/common/host/SystemClock.h>
#endif
