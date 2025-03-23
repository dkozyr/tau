#include "tau/crypto/Hmac.h"
#include <openssl/hmac.h>
#include <openssl/err.h>

namespace tau::crypto {

bool HmacSha1(const BufferViewConst& view, const std::string_view& password, uint8_t* output) {
    uint32_t size = 0;
    if(!HMAC(EVP_sha1(), password.data(), (int)password.size(), view.ptr, view.size, output, &size)) {
        ERR_clear_error();
        return false;
    }
    return true;
}

}
