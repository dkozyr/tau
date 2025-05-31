#pragma once

#include <openssl/x509v3.h>
#include <vector>
#include <string>

namespace tau::crypto {

class Certificate {
public:
    using Digest = std::vector<uint8_t>;

    struct Options {
        std::string cert;
        std::string key;
    };

    struct OptionsSelfSigned {
        const Certificate& ca;
        std::string cn = "Tau self-signed certificate";
        std::string sna = "IP:127.0.0.1";
    };

public:
    Certificate();
    Certificate(Options&& options);
    Certificate(OptionsSelfSigned&& options);
    ~Certificate();

    X509* GetCertificate() const { return _certificate; }
    EVP_PKEY* GetPrivateKey() const { return _private_key; }

    std::vector<uint8_t> GetCertificateBuffer() const;
    std::vector<uint8_t> GetPrivateKeyBuffer() const;

    std::string GetDigestSha256String() const;
    Digest GetDigestSha256() const;

private:
    void CreateCertificate();
    void CreateCertificateFromRequest(X509_REQ* request);
    void GeneratePrivateKey();

    static bool SignWithCA(X509* certificate, const Certificate& ca);
    static void SetCommonName(X509* certificate, const std::string& sn);
    static void SetSubjectAltName(X509_REQ* request, const std::string& san);

    static std::vector<uint8_t> GetDataFromBio(BIO* bio);

private:
    X509* _certificate = nullptr;
    EVP_PKEY* _private_key = nullptr;
};

}
