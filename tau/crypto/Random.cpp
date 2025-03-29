#include "tau/crypto/Random.h"
#include "tau/common/Base64.h"
#include <openssl/rand.h>
#include <vector>

namespace tau::crypto {

bool RandomBytes(uint8_t* ptr, size_t size) {
    return (1 == RAND_bytes(ptr, size));
}

std::string RandomBase64(size_t size) {
    auto size_aligned = DivCeil(size * 6, 8);
    std::vector<uint8_t> data(size_aligned, '.');
    RandomBytes(data.data(), data.size());
    return Base64Encode(data.data(), data.size()).substr(0, size);
}

}
