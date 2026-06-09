#include "tau/crypto/Certificate.h"
#include "tau/common/String.h"
#include "tau/common/Log.h"
// #include "tau/common/Exception.h"
// #include <mbedtls/oid.h>
#include <cstring>
#include <ctime>

namespace tau::crypto {

Certificate::Certificate() {
    mbedtls_x509_crt_init(&_certificate);
    Init();
    GeneratePrivateKey();
    CreateSelfSigned("Self-signed");
}

// Certificate::Certificate(Options&& options) {
//     mbedtls_x509_crt_init(&_certificate);
//     mbedtls_pk_init(&_key);
//     Init();

//     ThrowOnError(
//         mbedtls_x509_crt_parse_file(&_certificate, options.cert.data()),
//         "Failed to load certificate");

//     ThrowOnError(
//         mbedtls_pk_parse_keyfile(&_key, options.key.data(), nullptr),
//         "Failed to load private key");
// }

// Certificate::Certificate(OptionsSelfSigned&& options) {
//     mbedtls_x509_crt_init(&_certificate);
//     mbedtls_pk_init(&_key);
//     Init();
//     GeneratePrivateKey();

//     mbedtls_x509write_cert crt;
//     mbedtls_x509write_crt_init(&crt);

//     mbedtls_x509write_crt_set_md_alg(&crt, MBEDTLS_MD_SHA256);
//     mbedtls_x509write_crt_set_subject_key(&crt, &_key);
//     mbedtls_x509write_crt_set_issuer_key(&crt,
//         const_cast<mbedtls_pk_context*>(options.ca.GetPrivateKey()));

//     mbedtls_x509write_crt_set_subject_name(&crt, options.cn.data());
//     mbedtls_x509write_crt_set_issuer_name(&crt,
//         options.ca.GetCertificate()->subject.val.p);

//     mbedtls_x509write_crt_set_version(&crt, MBEDTLS_X509_CRT_VERSION_3);
//     mbedtls_x509write_crt_set_serial(&crt, nullptr, 0);

//     mbedtls_x509write_crt_set_validity(&crt,
//         "20240101000000", "20300101000000");

//     if (!options.san.empty()) {
//         mbedtls_x509write_crt_set_extension(
//             &crt,
//             MBEDTLS_OID_SUBJECT_ALT_NAME,
//             MBEDTLS_OID_SIZE(MBEDTLS_OID_SUBJECT_ALT_NAME),
//             0,
//             (const unsigned char*)options.san.data(),
//             options.san.size());
//     }

//     uint8_t buf[2048];
//     int len = mbedtls_x509write_crt_der(
//         &crt, buf, sizeof(buf),
//         mbedtls_ctr_drbg_random, &_drbg);

//     ThrowOnError(len, "Certificate signing failed");

//     ThrowOnError(
//         mbedtls_x509_crt_parse(&_certificate,
//             buf + sizeof(buf) - len, len),
//         "Certificate parse failed");

//     mbedtls_x509write_crt_free(&crt);
// }

Certificate::~Certificate() {
    mbedtls_x509_crt_free(&_certificate);
    mbedtls_pk_free(&_key);
    mbedtls_ctr_drbg_free(&_drbg);
    mbedtls_entropy_free(&_entropy);
}

void Certificate::Init() {
    mbedtls_entropy_init(&_entropy);
    mbedtls_ctr_drbg_init(&_drbg);

    if(auto error = mbedtls_ctr_drbg_seed(&_drbg, mbedtls_entropy_func, &_entropy, nullptr, 0)) {
        TAU_LOG_ERROR("mbedtls_ctr_drbg_seed failed, error: " << error);
    }
}

void Certificate::GeneratePrivateKey() {
    mbedtls_pk_init(&_key);
    if(auto error = mbedtls_pk_setup(&_key, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY))) {
        TAU_LOG_ERROR("mbedtls_pk_setup failed, error: " << error);
    }

    if(auto error = mbedtls_ecp_gen_key(MBEDTLS_ECP_DP_SECP256R1, mbedtls_pk_ec(_key), mbedtls_ctr_drbg_random, &_drbg)) {
        TAU_LOG_ERROR("mbedtls_ecp_gen_key failed, error: " << error);
    }

    // _key_pem.resize(1024);
    // auto key_size = mbedtls_pk_write_key_pem(&_key, _key_pem.data(), _key_pem.size());
    // if(key_size > 0) {
    //     _key_pem.resize(key_size);
    // } else {
    //     TAU_LOG_ERROR("mbedtls_pk_write_key_pem failed, error: " << key_size);
    //     {
    //         char err[128];
    //         mbedtls_strerror(key_size, err, sizeof(err));
    //         TAU_LOG_ERROR("pem write failed: " << key_size << ", message: " << err);
    //     }
    //     _key_pem.clear();
    // }
}

Certificate::DataBuffer Certificate::GetCertificateBuffer() const {
    return _certificate_der;
}

// Certificate::DataBuffer Certificate::GetCertificateBuffer() const {
//     DataBuffer buffer(2048);
//     auto size = mbedtls_x509_crt_info(reinterpret_cast<char*>(buffer.data()), buffer.size(), "", &_certificate);
//     if(size > 0) {
//         buffer.resize(size);
//         TAU_LOG_INFO("cert info: " << reinterpret_cast<char*>(buffer.data()));
//     } else {
//         TAU_LOG_ERROR("mbedtls_x509_crt_info failed, error: " << size);
//         buffer.clear();
//     }
//     return buffer;
// }

Certificate::DataBuffer Certificate::GetPrivateKeyBuffer() const {
    DataBuffer buffer(512);
    if(auto error = mbedtls_pk_write_key_pem(&_key, buffer.data(), buffer.size())) {
        TAU_LOG_ERROR("mbedtls_pk_write_key_pem failed, error: " << error);
        buffer.clear();
    } else {
        buffer.resize(strlen(reinterpret_cast<const char*>(buffer.data())) + 1);
    }
    return buffer;
}

Certificate::Digest Certificate::GetDigestSha256() const {
    Digest digest(kSha256Size);
    mbedtls_sha256(_certificate_der.data(), _certificate_der.size(), digest.data(), 0);
    return digest;
}

Certificate::DigestStr Certificate::GetDigestSha256String() const {
    auto digest = GetDigestSha256();
    DigestStr digest_str;
    ToHexDump(digest.data(), digest.size(), digest_str, ":");
    return digest_str;
}

// static size_t asn1_len_len(size_t len) {
//     return (len < 128) ? 1 : (1 + (len > 0xFF ? 2 : 1));
// }

static uint8_t* WriteAsn1Size(uint8_t* p, size_t size) {
    if(size < 128) {
        *--p = (uint8_t)size;
    } else {
        if(size <= 0xFF) {
            *--p = (uint8_t)size;
            *--p = 0x81;
        } else {
            *--p = (uint8_t)(size & 0xFF);
            *--p = (uint8_t)(size >> 8);
            *--p = 0x82;
        }
    }
    return p;
}

// static size_t write_san_dns(uint8_t* buf,
//                             size_t buf_size,
//                             const char* dns) {
//     size_t dns_len = strlen(dns);
//     uint8_t* p = buf + buf_size;

//     // IA5String
//     p -= dns_len;
//     memcpy(p, dns, dns_len);
//     p = WriteAsn1Size(p, dns_len);
//     *--p = 0x16; // IA5String

//     size_t inner_len = (buf + buf_size) - p;

//     // [2] context-specific, primitive
//     p = WriteAsn1Size(p, inner_len);
//     *--p = 0x82; // [2]

//     return (buf + buf_size) - p;
// }

static size_t WriteSanLocalhost(uint8_t* buf, size_t buf_size) {
    const uint8_t ip[4] = {127, 0, 0, 1};

    uint8_t* p = buf + buf_size;

    // OCTET STRING (4 bytes)
    p -= 4;
    memcpy(p, ip, 4);
    p = WriteAsn1Size(p, 4);
    *--p = 0x04; // OCTET STRING

    size_t inner_len = (buf + buf_size) - p;

    // [7] context-specific
    p = WriteAsn1Size(p, inner_len);
    *--p = 0x87; // [7]

    return (buf + buf_size) - p;
}

static size_t BuildSubjectAltName(uint8_t* out, size_t out_size) {
    uint8_t tmp[64];
    uint8_t* p = tmp + sizeof(tmp);

    size_t total = WriteSanLocalhost(tmp, sizeof(tmp));
    p -= total;

    // SEQUENCE
    p = WriteAsn1Size(p, total);
    *--p = 0x30;

    size_t seq_len = (tmp + sizeof(tmp)) - p;
    memcpy(out, p, seq_len);
    return seq_len;
}

void Certificate::CreateSelfSigned(const etl::string_view& cn) {
    mbedtls_x509write_cert crt;
    mbedtls_x509write_crt_init(&crt);

    mbedtls_x509write_crt_set_version(&crt, MBEDTLS_X509_CRT_VERSION_3);
    mbedtls_x509write_crt_set_md_alg(&crt, MBEDTLS_MD_SHA256);

    // subject == issuer (self-signed)
    mbedtls_x509write_crt_set_subject_key(&crt, &_key);
    mbedtls_x509write_crt_set_issuer_key(&crt, &_key);

    // CN
    {
        char subject[128];
        snprintf(subject, sizeof(subject), "CN=%.*s", (int)cn.size(), cn.data());

        if(auto error = mbedtls_x509write_crt_set_subject_name(&crt, subject)) {
            TAU_LOG_ERROR("mbedtls_x509write_crt_set_subject_name failed, error: " << error);
            return;
        }

        if(auto error = mbedtls_x509write_crt_set_issuer_name(&crt, subject)) {
            TAU_LOG_ERROR("mbedtls_x509write_crt_set_issuer_name failed, error: " << error);
            return;
        }
    }

    // // serial
    uint8_t serial[8];
    mbedtls_ctr_drbg_random(&_drbg, serial, sizeof(serial));
    serial[0] &= 0x7F; // Ensure positive (clear MSB)

    if(auto error = mbedtls_x509write_crt_set_serial_raw(&crt, serial, sizeof(serial))) {
        TAU_LOG_ERROR("mbedtls_x509write_crt_set_serial failed, error: " << error);
        return;
    }

    // validity (UTC, YYYYMMDDhhmmss)
    if(auto error = mbedtls_x509write_crt_set_validity(&crt, "20260301000000", "20360301000000")) {
        TAU_LOG_ERROR("mbedtls_x509write_crt_set_validity failed, error: " << error);
        return;
    }

    // ---------- SubjectAltName ----------
    uint8_t san_der[64];
    const auto san_len = BuildSubjectAltName(san_der, sizeof(san_der));

    static constexpr const char* OID_SUBJECT_ALT_NAME = "2.5.29.17";

    if(auto error = mbedtls_x509write_crt_set_extension(&crt,
            OID_SUBJECT_ALT_NAME,
            strlen(OID_SUBJECT_ALT_NAME),
            0, san_der, san_len)) {
        TAU_LOG_ERROR("mbedtls_x509write_crt_set_extension failed, error: " << error);
        return;
    }

    // ---------- sign ----------
    _certificate_der.resize(_certificate_der.capacity());
    auto size = mbedtls_x509write_crt_der(&crt, _certificate_der.data(), _certificate_der.size(), mbedtls_ctr_drbg_random, &_drbg);
    if(size > 0) {
        // DER is written to the end, move it
        memmove(_certificate_der.data(), _certificate_der.data() + (_certificate_der.capacity() - size), size);
        _certificate_der.resize(size);
    } else {
        TAU_LOG_ERROR("mbedtls_x509write_crt_der failed, error: " << size);
        return;
    }

    //TODO: do we need it?
    // if(auto error = mbedtls_x509_crt_parse(&_certificate, der + sizeof(der) - size, size)) {
    //     TAU_LOG_ERROR("mbedtls_x509_crt_parse failed, error: " << error);
    //     return;
    // }

    mbedtls_x509write_crt_free(&crt);
}

}
