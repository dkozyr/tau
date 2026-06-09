#pragma once

#include <mbedtls/x509_crt.h>
#include <mbedtls/x509_csr.h>
// #include <mbedtls/x509write.h>
#include <mbedtls/pk.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/sha256.h>
#include <etl/vector.h>
#include <etl/string.h>
#include <etl/string_view.h>

namespace tau::crypto {

class Certificate {
public:
    inline static constexpr size_t kSha256Size = 32;

    using Digest = etl::vector<uint8_t, kSha256Size>;
    using DigestStr = etl::string<3 * kSha256Size>;
    using DataBuffer = etl::vector<uint8_t, 1024>;

    // struct Options {
    //     etl::string_view cert;
    //     etl::string_view key;
    // };

    // struct OptionsSelfSigned {
    //     const Certificate& ca;
    //     etl::string_view cn  = "Tau self-signed certificate";
    //     etl::string_view san = "IP:127.0.0.1";
    // };

public:
    Certificate();
    // Certificate(Options&& options);
    // Certificate(OptionsSelfSigned&& options);
    ~Certificate();

    const mbedtls_x509_crt* GetCertificate() const { return &_certificate; }
    const mbedtls_pk_context* GetPrivateKey() const { return &_key; }

    //TODO: use reference to avoid copy?
    DataBuffer GetCertificateBuffer() const;
    DataBuffer GetPrivateKeyBuffer() const;

    Digest GetDigestSha256() const;
    DigestStr GetDigestSha256String() const;

private:
    void Init();
    void GeneratePrivateKey();
    void CreateSelfSigned(const etl::string_view& cn);

private:
    mbedtls_x509_crt _certificate;
    mbedtls_pk_context _key;

    mbedtls_entropy_context _entropy;
    mbedtls_ctr_drbg_context _drbg;

    DataBuffer _certificate_der;
};

}
