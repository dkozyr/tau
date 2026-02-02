#include "tau/crypto/Hmac.h"
#include <openssl/hmac.h>

namespace tau::crypto {

bool HmacSha1(const BufferViewConst& view, const std::string_view& password, uint8_t* output) {
    uint32_t size = 0;
    return HMAC(EVP_sha1(), password.data(), (int)password.size(), view.ptr, view.size, output, &size);
}

bool HmacSha256(const BufferViewConst& view, const std::string_view& password, uint8_t* output) {
    uint32_t size = 0;
    return HMAC(EVP_sha256(), password.data(), (int)password.size(), view.ptr, view.size, output, &size);
}

}
