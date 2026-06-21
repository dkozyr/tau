#pragma once

#include "tau/crypto/Certificate.h"
#include "tau/srtp/KeyMaterial.h"
#include "tau/memory/Buffer.h"
#include "mbedtls/ssl.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/pk.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include <etl/string.h>
#include <etl/string_view.h>
#include <etl/string_stream.h>
#include <functional>
#include <optional>

namespace tau::dtls {

class Session {
public:
//     static constexpr auto kSrtpProfilesDefault = "SRTP_AES128_CM_SHA1_80:SRTP_AES128_CM_SHA1_32"; //SRTP_AEAD_AES_128_GCM:SRTP_AEAD_AES_256_GCM
//     static constexpr auto kFailureTimeout = 10 * kSec;

    enum Type {
        kServer,
        kClient
    };

    enum State {
        kWaiting,
        kConnecting,
        kConnected,
        kFailed
    };

    // https://datatracker.ietf.org/doc/html/draft-ietf-avt-dtls-srtp-00#section-3.2.2
    // https://www.rfc-editor.org/rfc/rfc7714.html#section-14.2
    enum SrtpProfile : uint8_t {
        kAes128CmSha1_80 = 1,
        kAes128CmSha1_32 = 2,
        // kAes256CmSha1_80 = 3,
        // kAes256CmSha1_32 = 4,
        // kAeadAes128Gcm   = 7,
        // kAeadAes256Gcm   = 8,
    };

    struct Dependencies {
        Allocator& udp_allocator;
        crypto::Certificate& certificate;
    };

    struct Options{
        Type type;
        // etl::string<128> srtp_profiles; //TODO: check capacity
        etl::string_view remote_peer_cert_digest;
        etl::string_view log_ctx;
    };

    using Callback = std::function<void(Buffer&&)>;
    using StateCallback = std::function<void(State)>;

public:
    Session(Dependencies&& deps, Options&& options);
    ~Session();

    void SetSendCallback(Callback callback) { _send_callback = std::move(callback); }
    void SetRecvCallback(Callback callback) { _recv_callback = std::move(callback); }
    void SetStateCallback(StateCallback callback) { _state_callback = std::move(callback); }

    void Process();
    void Stop();

    bool Send(Buffer&& packet);
    bool Send(const BufferViewConst& packet);
    void Recv(Buffer&& packet);

//     std::optional<Timepoint> GetTimeout();

    std::optional<SrtpProfile> GetSrtpProfile() const;
    srtp::KeyMaterial GetKeyingMaterial(bool encryption) const;

private:
    bool Init();
    void Deinit();

    static int SendCallback(void* ctx, const uint8_t* buffer, size_t size);
    static int RecvCallback(void* ctx, uint8_t* buffer, size_t size);

    static void SetTimer(void* ctx, uint32_t int_ms, uint32_t fin_ms);
    static int GetTimer(void* ctx);

    static int OnVerifyPeerStatic(void* data, mbedtls_x509_crt* crt, int depth, uint32_t* flags);
    int OnVerifyPeer(mbedtls_x509_crt* crt, int depth, uint32_t* flags);

    static void OnExportKeysStatic(void* data, mbedtls_ssl_key_export_type type,
        const uint8_t* secret, size_t secret_len,
        const uint8_t client_random[32], const uint8_t server_random[32],
        mbedtls_tls_prf_types tls_prf_type);
    void OnExportKeys(mbedtls_ssl_key_export_type type,
        const uint8_t* secret, size_t secret_len,
        const uint8_t client_random[32], const uint8_t server_random[32],
        mbedtls_tls_prf_types tls_prf_type);


    static void DebugCallback(void*, int level, const char* file, int line, const char* msg);

private:
    Dependencies _deps;
    const Options _options;

    mbedtls_ssl_context _ctx;
    mbedtls_ssl_config _conf{};
    mbedtls_x509_crt _cert{};
    mbedtls_pk_context _key{};
    mbedtls_entropy_context _entropy{};
    mbedtls_ctr_drbg_context _drbg{};

    etl::vector<uint8_t, 1500> _rx;
    etl::vector<uint8_t, 1500> _tx;

    uint32_t _timer_int_ms = 0;
    uint32_t _timer_fin_ms = 0;
    Timepoint _timer_start{};
    bool _timer_active = false;

    KeyMaterial _secret;
    KeyMaterial _randoms;

    State _state = State::kWaiting;

    Callback _send_callback;
    Callback _recv_callback;
    StateCallback _state_callback;
};

etl::string_stream& operator<<(etl::string_stream& ss, const Session::State& x);

}
