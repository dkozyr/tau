#pragma once

#ifdef ESP_PLATFORM
    #include <tau/crypto/mbed/RandomBytes.h>
#else
    #include <tau/crypto/host/RandomBytes.h>
#endif

#include <etl/string.h>

namespace tau::crypto {

etl::istring& RandomBase64(etl::istring& output, size_t size);

}
