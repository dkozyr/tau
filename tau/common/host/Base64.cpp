#include "tau/common/Base64.h"
#include "tau/common/Math.h"
#include <openssl/evp.h>

namespace tau {

etl::istring& Base64Decode(etl::string_view input, etl::istring& decoded) {
    decoded.clear();
    const size_t max_decoded_size = (input.size() / 4) * 3;
    if(max_decoded_size <= decoded.capacity()) { 
        decoded.resize(max_decoded_size);
        const auto result = EVP_DecodeBlock(
            reinterpret_cast<uint8_t*>(decoded.data()),
            reinterpret_cast<const uint8_t*>(input.data()),
            input.size()
        );
        if(static_cast<size_t>(result) == max_decoded_size) {
            if((input.size() >= 2) && (input[input.size() - 2] == '=')) {
                decoded.resize(max_decoded_size - 2);
            } else if((input.size() >= 1) && (input[input.size() - 1] == '=')) {
                decoded.resize(max_decoded_size - 1);
            }
        } else {
            decoded.clear();
        }
    }
    return decoded;
}

etl::istring& Base64Encode(etl::string_view input, etl::istring& encoded) {
    const auto encoded_size = 4 * DivCeil<int>(input.size(), 3);
    if(static_cast<size_t>(encoded_size) <= encoded.capacity()) {
        encoded.resize(encoded_size, '=');
        const auto result = EVP_EncodeBlock(
            reinterpret_cast<uint8_t*>(encoded.data()),
            reinterpret_cast<const uint8_t*>(input.data()),
            input.size()
        );
        if(encoded_size != result) {
            encoded.clear();
        }
    } else {
        encoded.clear();
    }
    return encoded;
}

}
