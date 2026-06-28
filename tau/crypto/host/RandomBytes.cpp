#include <tau/crypto/host/RandomBytes.h>
#include <openssl/rand.h>

namespace tau::crypto {

bool RandomBytes(uint8_t* ptr, size_t size) {
    return RAND_bytes(ptr, size);
}

}
