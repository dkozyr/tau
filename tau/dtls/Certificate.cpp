#include "tau/dtls/Certificate.h"
#include "tau/common/String.h"
#include "tau/common/Exception.h"
#include <openssl/rsa.h>
#include <openssl/err.h>

namespace tau::dtls {

Certificate::Certificate() {
    GeneratePrivateKey();

    _certificate = X509_new();
    X509_set_version(_certificate, 2);
    ASN1_INTEGER_set(X509_get_serialNumber(_certificate), 1);
    X509_gmtime_adj(X509_get_notBefore(_certificate), 0);
    X509_gmtime_adj(X509_get_notAfter(_certificate), 30 * 24 * 3600); // 30 days validity
    X509_set_pubkey(_certificate, _private_key);

    X509_sign(_certificate, _private_key, EVP_sha256());
}

Certificate::~Certificate() {
    X509_free(_certificate);
    EVP_PKEY_free(_private_key);
}

std::string Certificate::GetDigestSha256String() const {
    auto digest = GetDigestSha256();
    return ToHexDump(digest.data(), digest.size(), ':');
}

Certificate::Digest Certificate::GetDigestSha256() const {
    Digest digest(EVP_MAX_MD_SIZE);
    uint32_t size = 0;
    if(X509_digest(_certificate, EVP_sha256(), digest.data(), &size)) {
        digest.resize(size);
    }
    return digest;
}

void Certificate::GeneratePrivateKey() {
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
    if(!ctx) {
        TAU_EXCEPTION(std::runtime_error, "EVP_PKEY_CTX_new_id failed");
    }

    if(EVP_PKEY_keygen_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        TAU_EXCEPTION(std::runtime_error, "EVP_PKEY_CTX_new_id failed");
    }

    if(EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        TAU_EXCEPTION(std::runtime_error, "EVP_PKEY_CTX_set_rsa_keygen_bits failed");
    }

    if(EVP_PKEY_keygen(ctx, &_private_key) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        TAU_EXCEPTION(std::runtime_error, "EVP_PKEY_keygen failed");
    }

    EVP_PKEY_CTX_free(ctx);
}

}
