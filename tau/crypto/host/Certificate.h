#pragma once

#include <openssl/x509v3.h>
#include <etl/vector.h>
#include <etl/string.h>
#include <etl/string_view.h>

namespace tau::crypto {

class Certificate {
public:
    inline static constexpr size_t kSha256Size = 32;

    using Digest = etl::vector<uint8_t, kSha256Size>;
    using DigestStr = etl::string<3 * kSha256Size>;
    using DataBuffer = etl::vector<uint8_t, 2048>;

    struct Options {
        etl::string_view cert;
        etl::string_view key;
    };

    struct OptionsSelfSigned {
        const Certificate& ca;
        etl::string_view cn = "Tau self-signed certificate";
        etl::string_view sna = "IP:127.0.0.1";
    };

public:
    Certificate();
    Certificate(Options&& options);
    Certificate(OptionsSelfSigned&& options);
    ~Certificate();

    X509* GetCertificate() const { return _certificate; }
    EVP_PKEY* GetPrivateKey() const { return _private_key; }

    DataBuffer GetCertificateBuffer() const;
    DataBuffer GetPrivateKeyBuffer() const;

    DigestStr GetDigestSha256String() const;
    Digest GetDigestSha256() const;

private:
    void CreateCertificate();
    void CreateCertificateFromRequest(X509_REQ* request);
    void GeneratePrivateKey();

    static bool SignWithCA(X509* certificate, const Certificate& ca);
    static void SetCommonName(X509* certificate, const etl::string_view& sn);
    static void SetSubjectAltName(X509_REQ* request, const etl::string_view& san);

    static DataBuffer GetDataFromBio(BIO* bio);

private:
    X509* _certificate = nullptr;
    EVP_PKEY* _private_key = nullptr;
};

}
