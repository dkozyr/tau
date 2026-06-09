#pragma once

#ifdef ESP_PLATFORM
    #include <tau/common/mbed/lock/Barrier.h>
#else
    #include <tau/common/host/lock/Barrier.h>
#endif
