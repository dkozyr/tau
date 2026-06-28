#include <tau/crypto/mbed/RandomBytes.h>
#include <tau/crypto/mbed/Common.h>

namespace tau::crypto {

bool RandomBytes(uint8_t* ptr, size_t size) {
    return (0 == mbedtls_ctr_drbg_random(&g_mbedtls_ctr_drbg, ptr, size));
}

}
