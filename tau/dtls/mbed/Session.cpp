#include "tau/dtls/mbed/Session.h"
#include "tau/crypto/Certificate.h"
#include "tau/common/SteadyClock.h"
#include "tau/common/String.h"
#include "tau/common/Log.h"
#include "mbedtls/ssl.h"
#include <mbedtls/debug.h>
namespace tau::dtls {

static const int ciphersuites[] = {
    MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256,
    MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,
    0
};
static const mbedtls_ssl_srtp_profile srtp_profiles[] = {
    MBEDTLS_TLS_SRTP_AES128_CM_HMAC_SHA1_80,
    MBEDTLS_TLS_SRTP_AES128_CM_HMAC_SHA1_32,
    MBEDTLS_TLS_SRTP_UNSET
};

Session::Session(Dependencies&& deps, Options&& options)
    : _deps(deps)
    , _options(std::move(options))
{
    if(!Init()) {
        Deinit();
    }
}

Session::~Session() {
    Deinit();
}

void Session::Stop() {
    if(auto error = mbedtls_ssl_close_notify(&_ctx)) {
        TAU_LOG_WARNING("mbedtls_ssl_close_notify failed, error: " << error);
    }
}

void Session::Process() {
    switch(_state) {
        case State::kWaiting:
            if(_options.type == Type::kClient) {
                _state = State::kConnecting;
                _state_callback(_state);
            }
            break;
        case State::kConnecting:
            break;
        case State::kConnected:
            while(true) {
                auto packet = Buffer::Create(_deps.udp_allocator);
                auto view = packet.GetViewWithCapacity();
                auto recv_read = mbedtls_ssl_read(&_ctx, view.ptr, view.size);
                if(recv_read <= 0) {
                    break;
                }
                packet.SetSize(recv_read);
                _recv_callback(std::move(packet));
            }
            return;
        case State::kFailed: return;
    }

    const auto result = mbedtls_ssl_handshake(&_ctx);
    if(result == 0) {
        _state = State::kConnected;
        _state_callback(_state);
        return;
    }

    if((result == MBEDTLS_ERR_SSL_WANT_READ) || (result == MBEDTLS_ERR_SSL_WANT_WRITE)) {
        return;
    }

    TAU_LOG_INFO("failed result: " << result);
    _state = State::kFailed;
    _state_callback(_state);
}

bool Session::Send(Buffer&& packet) {
    return Send(ToConst(packet.GetView()));
}

bool Session::Send(const BufferViewConst& view) {
    auto bytes = mbedtls_ssl_write(&_ctx, view.ptr, view.size);
    if(bytes <= 0) {
        TAU_LOG_WARNING("mbedtls_ssl_write failed, error: " << bytes);
    }
    return (bytes > 0);
}

void Session::Recv(Buffer&& packet) {
    if((_options.type == Type::kServer) && (_state == State::kWaiting)) {
        _state = State::kConnecting;
        _state_callback(_state);
    }

    auto view = packet.GetView();
    _rx.insert(_rx.end(), view.ptr, view.ptr + view.size);
}

// std::optional<Timepoint> Session::GetTimeout() {
//     if (_state != State::kConnecting)
//         return std::nullopt;

//     uint32_t ms = mbedtls_ssl_get_timer(&_ssl);
//     if (ms == 0 || ms > kFailureTimeout.count())
//         return std::nullopt;

//     return Timepoint{ms};
// }

std::optional<Session::SrtpProfile> Session::GetSrtpProfile() const {
    if(_secret.empty()) {
        return std::nullopt;
    }
    mbedtls_dtls_srtp_info info{};
    mbedtls_ssl_get_dtls_srtp_negotiation_result(&_ctx, &info);
    switch(info.private_chosen_dtls_srtp_profile) {
        case MBEDTLS_TLS_SRTP_AES128_CM_HMAC_SHA1_80:
            return SrtpProfile::kAes128CmSha1_80;
        case MBEDTLS_TLS_SRTP_AES128_CM_HMAC_SHA1_32:
            return SrtpProfile::kAes128CmSha1_32;
        default:
            break;
    }
    return std::nullopt;
}

Session::KeyMaterial Session::GetKeyingMaterial(bool encryption) const {
    auto profile = GetSrtpProfile();
    if(!profile) {
        return {};
    }

    // // const auto& [key, salt] = kProfileKeyingMaterialSize.at(*profile);
    constexpr size_t key_size = 16;
    constexpr size_t salt_size = 14;

    KeyMaterial keying_material(2 * (key_size + salt_size));
    auto error = mbedtls_ssl_tls_prf(MBEDTLS_SSL_TLS_PRF_SHA256,
        _secret.data(), _secret.size(),
        "EXTRACTOR-dtls_srtp",
        _randoms.data(), _randoms.size(),
        keying_material.data(), keying_material.size()
    );
    if(error) {
        TAU_LOG_WARNING("mbedtls_ssl_tls_prf failed, error: " << error);
        return {};
    }

    // https://www.rfc-editor.org/rfc/rfc5764.html#section-4.2
    if((encryption && (_options.type == Type::kClient)) || (!encryption && (_options.type == Type::kServer))) {
        std::memcpy(keying_material.data() + key_size, keying_material.data() + 2 * key_size, salt_size);
    } else {
        std::memcpy(keying_material.data(), keying_material.data() + key_size, key_size);
        std::memcpy(keying_material.data() + key_size, keying_material.data() + 2 * key_size + salt_size, salt_size);
    }
    keying_material.resize(key_size + salt_size);
    return keying_material;
}

int Session::OnVerifyPeerStatic(void* data, mbedtls_x509_crt* crt, int depth, uint32_t* flags) {
    auto self = reinterpret_cast<Session*>(data);
    if(self) {
        return self->OnVerifyPeer(crt, depth, flags);
    }
    return -1;
}

int Session::OnVerifyPeer(mbedtls_x509_crt* crt, int depth, uint32_t* flags) {
    *flags &= ~(MBEDTLS_X509_BADCERT_NOT_TRUSTED); // self-signed certificate
    if(depth == 0) { // last stage - peer certificate
        crypto::Certificate::Digest digest(crypto::Certificate::kSha256Size);
        mbedtls_sha256(crt->raw.p, crt->raw.len, digest.data(), 0);

        crypto::Certificate::DigestStr actual_digest;
        ToHexDump(digest.data(), digest.size(), actual_digest, ":");

        const auto& target_digest = _options.remote_peer_cert_digest;
        if(target_digest != actual_digest) {
            TAU_LOG_WARNING("Wrong certificate, digest: " << actual_digest);
            *flags |= MBEDTLS_X509_BADCERT_OTHER; 
            return -1;
        }
    }
    return 0;
}

void Session::OnExportKeysStatic(void* data, mbedtls_ssl_key_export_type type,
    const uint8_t* secret, size_t secret_len,
    const uint8_t client_random[32], const uint8_t server_random[32],
    mbedtls_tls_prf_types tls_prf_type) {
    auto self = reinterpret_cast<Session*>(data);
    if(self) {
        return self->OnExportKeys(type, secret, secret_len, client_random, server_random, tls_prf_type);
    }
}

void Session::OnExportKeys(mbedtls_ssl_key_export_type type,
    const uint8_t* secret, size_t secret_len,
    const uint8_t client_random[32], const uint8_t server_random[32],
    mbedtls_tls_prf_types tls_prf_type) {

    if(secret_len > _secret.capacity()) {
        TAU_LOG_WARNING("Wrong secret len: " << secret_len);
        return;
    }

    _secret.resize(secret_len);
    memcpy(_secret.data(), secret, secret_len);

    _randoms.resize(2 * 32);
    memcpy(_randoms.data(),      client_random, 32);
    memcpy(_randoms.data() + 32, server_random, 32);
}

bool Session::Init() {
    mbedtls_ssl_init(&_ctx);
    mbedtls_ssl_config_init(&_conf);
    mbedtls_x509_crt_init(&_cert);
    mbedtls_pk_init(&_key);
    mbedtls_entropy_init(&_entropy);
    mbedtls_ctr_drbg_init(&_drbg);

    if(auto error = mbedtls_ctr_drbg_seed(&_drbg, mbedtls_entropy_func, &_entropy, nullptr, 0)) {
        TAU_LOG_WARNING("mbedtls_ctr_drbg_seed failed, error: " << error);
        return false;
    }

    auto cert = _deps.certificate.GetCertificateBuffer();
    if(auto error = mbedtls_x509_crt_parse(&_cert, cert.data(), cert.size())) {
        TAU_LOG_WARNING("mbedtls_x509_crt_parse failed, error: " << error);
        return false;
    }
    auto key_pem = _deps.certificate.GetPrivateKeyBuffer();
    if(auto error = mbedtls_pk_parse_key(&_key, key_pem.data(), key_pem.size(), nullptr, 0, mbedtls_ctr_drbg_random, &_drbg)) {
        TAU_LOG_WARNING("mbedtls_pk_parse_key failed, error: " << error);
        return false;
    }

    const auto type = (_options.type == Type::kServer) ? MBEDTLS_SSL_IS_SERVER : MBEDTLS_SSL_IS_CLIENT;
    if(auto error = mbedtls_ssl_config_defaults(&_conf, type, MBEDTLS_SSL_TRANSPORT_DATAGRAM, MBEDTLS_SSL_PRESET_DEFAULT)) {
        TAU_LOG_WARNING("mbedtls_ssl_config_defaults failed, error: " << error);
        return false;
    }

    mbedtls_ssl_conf_ciphersuites(&_conf, ciphersuites);

    if(_options.remote_peer_cert_digest.empty()) {
        mbedtls_ssl_conf_authmode(&_conf, MBEDTLS_SSL_VERIFY_NONE);
    } else {
        mbedtls_ssl_conf_authmode(&_conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
        mbedtls_ssl_conf_verify(&_conf, OnVerifyPeerStatic, this);
    }
    mbedtls_ssl_conf_own_cert(&_conf, &_cert, &_key);

    mbedtls_ssl_conf_rng(&_conf, mbedtls_ctr_drbg_random, &_drbg);
    mbedtls_ssl_conf_dtls_cookies(&_conf, nullptr, nullptr, nullptr);

    mbedtls_ssl_conf_dtls_srtp_protection_profiles(&_conf, srtp_profiles);

    // mbedtls_ssl_get_dtls_srtp_key_material(_ctx);
    mbedtls_ssl_set_export_keys_cb(&_ctx, OnExportKeysStatic, this);

    if(auto error = mbedtls_ssl_setup(&_ctx, &_conf)) {
        TAU_LOG_WARNING("mbedtls_ssl_setup failed, error: " << error);
        return false;
    }

    mbedtls_ssl_conf_dbg(&_conf, DebugCallback, nullptr);
    mbedtls_debug_set_threshold(1); // 1..4

    mbedtls_ssl_set_bio(&_ctx, this, &Session::SendCallback, &Session::RecvCallback, nullptr);
    mbedtls_ssl_set_timer_cb(&_ctx, this, &Session::SetTimer, &Session::GetTimer);
    return true;
}

void Session::Deinit() {
    mbedtls_ssl_free(&_ctx);
    mbedtls_ssl_config_free(&_conf);
    mbedtls_x509_crt_free(&_cert);
    mbedtls_pk_free(&_key);
    mbedtls_ctr_drbg_free(&_drbg);
    mbedtls_entropy_free(&_entropy);
}

int Session::SendCallback(void* ctx, const uint8_t* buffer, size_t size) {
    auto self = static_cast<Session*>(ctx);
    auto packet = Buffer::Create(self->_deps.udp_allocator);
    memcpy(packet.GetView().ptr, buffer, size);
    packet.SetSize(size);
    self->_send_callback(std::move(packet));
    return size;
}

int Session::RecvCallback(void* ctx, uint8_t* buffer, size_t size) {
    auto self = static_cast<Session*>(ctx);
    if(self->_rx.empty()) {
        return MBEDTLS_ERR_SSL_WANT_READ;
    }

    auto bytes = std::min(size, self->_rx.size());
    memcpy(buffer, self->_rx.data(), bytes);
    self->_rx.erase(self->_rx.begin(), self->_rx.begin() + bytes);
    return bytes;
}

void Session::SetTimer(void* ctx, uint32_t int_ms, uint32_t fin_ms) {
    auto* self = static_cast<Session*>(ctx);
    if(fin_ms == 0) {
        // disable timer
        self->_timer_active = false;
        self->_timer_int_ms = 0;
        self->_timer_fin_ms = 0;
    } else {
        self->_timer_active = true;
        self->_timer_int_ms = int_ms;
        self->_timer_fin_ms = fin_ms;
        self->_timer_start = SteadyClock{}.Now(); //TODO: clock dependency
    }
}

int Session::GetTimer(void* ctx) {
    auto* self = static_cast<Session*>(ctx);
    if(!self->_timer_active) {
        return -1; // timer cancelled
    }

    auto elapsed_ms = DurationMsInt(self->_timer_start, SteadyClock{}.Now());
    if(elapsed_ms >= self->_timer_fin_ms) {
        return 2; // final timeout expired
    }
    if(elapsed_ms >= self->_timer_int_ms) {
        return 1; // intermediate timeout expired
    }
    return 0;
}

void Session::DebugCallback(void*, int level, const char* file, int line, const char* msg) {
    TAU_LOG_INFO(file << ":" << line << " | " << msg);
}

etl::string_stream& operator<<(etl::string_stream& ss, const Session::State& x) {
    switch(x) {
        case Session::State::kWaiting:    return ss << "waiting";
        case Session::State::kConnecting: return ss << "connecting";
        case Session::State::kConnected:  return ss << "connected";
        case Session::State::kFailed:     return ss << "failed";
    }
    return ss << "unknown";
}

}
