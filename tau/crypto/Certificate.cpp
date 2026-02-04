#include "tau/crypto/Certificate.h"
#include "tau/common/String.h"
#include "tau/common/Exception.h"
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>

namespace tau::crypto {

Certificate::Certificate() {
    GeneratePrivateKey();
    CreateCertificate();
    X509_set_pubkey(_certificate, _private_key);
    X509_sign(_certificate, _private_key, EVP_sha256());
}

Certificate::Certificate(Options&& options) {
    auto cert_file = fopen(options.cert.c_str(), "r");
    if(!cert_file) {
        TAU_EXCEPTION(std::runtime_error, "Certificate loading failed, cert: " << options.cert);
    }
    _certificate = PEM_read_X509(cert_file, nullptr, nullptr, nullptr);
    fclose(cert_file);

    auto key_file = fopen(options.key.c_str(), "r");
    if(!key_file) {
        X509_free(_certificate);
        TAU_EXCEPTION(std::runtime_error, "Certificate loading failed, key: " << options.key);
    }
    _private_key = PEM_read_PrivateKey(key_file, nullptr, nullptr, nullptr);
    fclose(key_file);
}

Certificate::Certificate(OptionsSelfSigned&& options) {
    GeneratePrivateKey();

    auto request = X509_REQ_new();
    X509_REQ_set_version(request, 0);
    X509_REQ_set_pubkey(request, _private_key);
    SetSubjectAltName(request, options.sna);
    X509_REQ_sign(request, _private_key, EVP_sha256());

    CreateCertificateFromRequest(request);
    X509_REQ_free(request);

    SetCommonName(_certificate, options.cn);
    if(!SignWithCA(_certificate, options.ca)) {
        X509_free(_certificate);
        EVP_PKEY_free(_private_key);
        TAU_EXCEPTION(std::runtime_error, "CA signing failed");
    }
}

Certificate::~Certificate() {
    X509_free(_certificate);
    EVP_PKEY_free(_private_key);
}

std::vector<uint8_t> Certificate::GetCertificateBuffer() const {
    auto bio = BIO_new(BIO_s_mem());
    PEM_write_bio_X509(bio, _certificate);
    return GetDataFromBio(bio);
}

std::vector<uint8_t> Certificate::GetPrivateKeyBuffer() const {
    auto bio = BIO_new(BIO_s_mem());
    PEM_write_bio_PrivateKey(bio, _private_key, nullptr, nullptr, 0, nullptr, nullptr);
    return GetDataFromBio(bio);
}

std::string Certificate::GetDigestSha256String() const {
    auto digest = GetDigestSha256();
    return ToHexDump(digest.data(), digest.size(), ":");
}

Certificate::Digest Certificate::GetDigestSha256() const {
    Digest digest(EVP_MAX_MD_SIZE);
    uint32_t size = 0;
    if(X509_digest(_certificate, EVP_sha256(), digest.data(), &size)) {
        digest.resize(size);
    }
    return digest;
}

void Certificate::CreateCertificate() {
    _certificate = X509_new();
    X509_set_version(_certificate, 2);
    ASN1_INTEGER_set(X509_get_serialNumber(_certificate), time(NULL));
    X509_gmtime_adj(X509_get_notBefore(_certificate), 0);
    X509_gmtime_adj(X509_get_notAfter(_certificate), 30 * 24 * 3600); // 30 days validity
}

void Certificate::CreateCertificateFromRequest(X509_REQ* request) {
    CreateCertificate();

    auto req_pubkey = X509_REQ_get_pubkey(request);
    X509_set_pubkey(_certificate, req_pubkey);
    EVP_PKEY_free(req_pubkey);

    auto extensions = (STACK_OF(X509_EXTENSION)*)X509_REQ_get_extensions(request);
    if(extensions) {
        for(auto i = 0; i < sk_X509_EXTENSION_num(extensions); ++i) {
            auto extension = sk_X509_EXTENSION_value(extensions, i);
            X509_add_ext(_certificate, extension, -1);
        }
        sk_X509_EXTENSION_pop_free(extensions, X509_EXTENSION_free);
    }
}

void Certificate::GeneratePrivateKey() {
    auto ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
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

std::vector<uint8_t> Certificate::GetDataFromBio(BIO* bio) {
    BUF_MEM* buffer;
    BIO_get_mem_ptr(bio, &buffer);

    std::vector<uint8_t> data;
    data.reserve(buffer->length);
    data.assign(buffer->data, buffer->data + buffer->length);

    BIO_free(bio);
    return data;
}

bool Certificate::SignWithCA(X509* certificate, const Certificate& ca) {
    X509_set_issuer_name(certificate, X509_get_subject_name(ca.GetCertificate()));
    X509_sign(certificate, ca.GetPrivateKey(), EVP_sha256());
    return true;
}

void Certificate::SetCommonName(X509* certificate, const std::string& sn) {
    auto name = X509_get_subject_name(certificate);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (uint8_t*)sn.c_str(), -1, -1, 0);
}

void Certificate::SetSubjectAltName(X509_REQ* request, const std::string& san) {
    X509V3_CTX ctx;
    X509V3_set_ctx_nodb(&ctx);
    X509V3_set_ctx(&ctx, nullptr, nullptr, request, nullptr, 0);

    auto extension = X509V3_EXT_conf_nid(nullptr, &ctx, NID_subject_alt_name, san.data());

    STACK_OF(X509_EXTENSION)* extensions = sk_X509_EXTENSION_new_null();
    sk_X509_EXTENSION_push(extensions, extension);
    X509_REQ_add_extensions(request, extensions);
    sk_X509_EXTENSION_pop_free(extensions, X509_EXTENSION_free);
}

}
