#include "tau/crypto/Md5.h"
#include <openssl/evp.h>

namespace tau::crypto {

bool Md5(const std::string_view& data, uint8_t* output) {
    return EVP_Q_digest(NULL, "MD5", NULL, data.data(), data.size(), output, NULL);
}

}
