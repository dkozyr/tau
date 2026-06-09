#pragma once

#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include <string_view>

namespace tau::crypto {

extern mbedtls_entropy_context g_mbedtls_entropy;
extern mbedtls_ctr_drbg_context g_mbedtls_ctr_drbg;

void Init(const std::string_view personalization);
void Deinit();

}
