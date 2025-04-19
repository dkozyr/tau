#include "tau/crypto/Certificate.h"
#include "tests/lib/Common.h"

namespace tau::crypto {

TEST(CertificateTest, SelfSigned) {
    Certificate cert;
    ASSERT_NE(nullptr, cert.GetCertificate());
    ASSERT_NE(nullptr, cert.GetPrivateKey());

    auto fingerprint = cert.GetDigestSha256();
    ASSERT_FALSE(fingerprint.empty());
    TAU_LOG_INFO("Fingerprint: " << cert.GetDigestSha256String());

    auto cert_data = cert.GetCertificateBuffer();
    ASSERT_EQ(924, cert_data.size());
 
    auto key_data = cert.GetPrivateKeyBuffer();
    ASSERT_EQ(1704, key_data.size());
}

}
