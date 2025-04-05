#include "tau/ice/Credentials.h"
#include "tau/crypto/Md5.h"

namespace tau::ice {

bool CalcLongTermPassword(const PeerCredentials& credentials, std::string_view realm, uint8_t* output) {
    const std::string data = credentials.ufrag + ":" + std::string{realm} + ":" + credentials.password;
    return crypto::Md5(data, output);
}

}
