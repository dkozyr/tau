#include "tau/dtls/Certificate.h"
#include "tests/lib/Common.h"

namespace tau::dtls {

TEST(CertificateTest, SelfSigned) {
    Certificate cert;
    ASSERT_NE(nullptr, cert.GetCertificate());
    ASSERT_NE(nullptr, cert.GetPrivateKey());
}

}
