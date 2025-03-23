#include "tau/crypto/Random.h"
#include <openssl/rand.h>

namespace tau::crypto {

bool RandomBytes(uint8_t* ptr, size_t size) {
    return (1 == RAND_bytes(ptr, size));
}

}
