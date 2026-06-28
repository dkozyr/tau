#pragma once

#ifdef ESP_PLATFORM
    #include <tau/common/mbed/Random.h>
#else
    #include <tau/common/host/Random.h>
#endif
