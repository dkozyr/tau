#pragma once

#include <openssl/x509v3.h>
#include <vector>
#include <string>

namespace tau::dtls {

class Certificate {
public:
    using Digest = std::vector<uint8_t>;

public:
    Certificate();
    ~Certificate();

    X509* GetCertificate() { return _certificate; }
    EVP_PKEY* GetPrivateKey() { return _private_key; }

    std::string GetDigestSha256String() const;
    Digest GetDigestSha256() const;

private:
    void GeneratePrivateKey();

private:
    X509* _certificate = nullptr;
    EVP_PKEY* _private_key = nullptr;
};

}
