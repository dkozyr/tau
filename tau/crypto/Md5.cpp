#include "tau/crypto/Md5.h"
#include <openssl/md5.h>

namespace tau::crypto {

bool Md5(const std::string_view& data, uint8_t* output) {
    MD5_CTX ctx;
    if(MD5_Init(&ctx)) {
        if(MD5_Update(&ctx, data.data(), data.size())) {
            return MD5_Final(output, &ctx);
        }
    }
    return false;
}

}
