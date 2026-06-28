#include <tau/crypto/mbed/Common.h>
#include <tau/common/Log.h>
#include <cstdint>

namespace tau::crypto {

mbedtls_entropy_context g_mbedtls_entropy;
mbedtls_ctr_drbg_context g_mbedtls_ctr_drbg;

void Init(const std::string_view personalization) {
    mbedtls_entropy_init(&g_mbedtls_entropy);
    mbedtls_ctr_drbg_init(&g_mbedtls_ctr_drbg);

    const auto ret = mbedtls_ctr_drbg_seed(
        &g_mbedtls_ctr_drbg, mbedtls_entropy_func, &g_mbedtls_entropy,
        reinterpret_cast<const uint8_t*>(personalization.data()),
        personalization.size());

    if(ret != 0) {
        TAU_LOG_ERROR("Failed in mbedtls_ctr_drbg_seed: " << ret);
    }
}

void Deinit() {
    mbedtls_ctr_drbg_free(&g_mbedtls_ctr_drbg);
    mbedtls_entropy_free(&g_mbedtls_entropy);
}

}
