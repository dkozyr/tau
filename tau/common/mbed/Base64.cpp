#include "tau/common/Base64.h"
#include "tau/common/Math.h"
#include <mbedtls/base64.h>

namespace tau {

etl::istring& Base64Decode(etl::string_view input, etl::istring& decoded) {
    const size_t target_size = DivCeil(input.size() * 6, 8);
    decoded.resize(target_size);
    memset(decoded.data(), 0, decoded.size());

    size_t output_len;
    const auto ret = mbedtls_base64_decode(
        reinterpret_cast<uint8_t*>(decoded.data()), decoded.size() + 1, &output_len,
        reinterpret_cast<const uint8_t*>(input.data()), input.size());
    if(0 == ret) {
        decoded.resize(output_len);
    } else {
        decoded.clear();
    }
    return decoded;
}

etl::istring& Base64Encode(etl::string_view input, etl::istring& encoded) {
    const size_t target_size = Align(Align(input.size() * 8, 6) / 6, 4);
    if(target_size <= encoded.capacity()) {
        encoded.resize(target_size);
        memset(encoded.data(), 0, encoded.size());

        size_t output_len;
        const auto ret = mbedtls_base64_encode(
            reinterpret_cast<uint8_t*>(encoded.data()), encoded.size() + 1, &output_len,
            reinterpret_cast<const uint8_t*>(input.data()), input.size());
        if(0 != ret) {
            encoded.clear();
        }
    } else {
        encoded.clear();
    }
    return encoded;
}

}
