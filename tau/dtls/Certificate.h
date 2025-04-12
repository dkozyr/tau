#pragma once

#include <openssl/x509v3.h>

namespace tau::dtls {

class Certificate {
public:
    Certificate();
    ~Certificate();

    X509* GetCertificate() { return _certificate; }
    EVP_PKEY* GetPrivateKey() { return _private_key; }

private:
    void GeneratePrivateKey();

private:
    X509* _certificate = nullptr;
    EVP_PKEY* _private_key = nullptr;
};

}
