#include "tau/ice/Credentials.h"
#include "tau/crypto/Md5.h"

namespace tau::ice {

bool CalcLongTermPassword(const PeerCredentials& credentials, etl::string_view realm, uint8_t* output) {
    crypto::Md5Hasher md5;
    md5.Update(credentials.ufrag);
    md5.Update(":");
    md5.Update(realm);
    md5.Update(":");
    md5.Update(credentials.password);
    return md5.Finalize(output);
}

}
