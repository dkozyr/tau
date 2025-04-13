#include "tau/dtls/Certificate.h"
#include "tests/lib/Common.h"

namespace tau::dtls {

TEST(CertificateTest, SelfSigned) {
    Certificate cert;
    ASSERT_NE(nullptr, cert.GetCertificate());
    ASSERT_NE(nullptr, cert.GetPrivateKey());

    auto fingerprint = cert.GetDigestSha256();
    ASSERT_FALSE(fingerprint.empty());
    TAU_LOG_INFO("Fingerprint: " << cert.GetDigestSha256String());
}

}
