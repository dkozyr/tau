#pragma once

#ifdef ESP_PLATFORM
    #include <tau/common/mbed/SteadyClock.h>
#else
    #include <tau/common/host/SteadyClock.h>
#endif
