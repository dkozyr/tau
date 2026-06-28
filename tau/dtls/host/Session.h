#pragma once

#include <tau/crypto/Certificate.h>
#include <tau/srtp/KeyMaterial.h>
#include <tau/memory/Buffer.h>
#include <openssl/ssl.h>
#include <etl/string_stream.h>
#include <functional>
#include <optional>

namespace tau::dtls {

class Session {
public:
    static constexpr auto kFailureTimeout = 10 * kSec;

    enum Type {
        kServer,
        kClient
    };

    enum State {
        kWaiting    = 0,
        kConnecting = 1,
        kConnected  = 2,
        kFailed     = 3
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
    static constexpr size_t kSrtpProfilesCount = 2;

    struct Dependencies {
        Allocator& udp_allocator;
        crypto::Certificate& certificate;
    };

    struct Options{
        Type type;
        etl::vector<SrtpProfile, 2> srtp_profiles;
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

    std::optional<Timepoint> GetTimeout();

    std::optional<SrtpProfile> GetSrtpProfile() const;
    srtp::KeyMaterial GetKeyingMaterial(bool encryption) const;

private:
    void ProcessPending();

    static int OnVerifyPeerStatic(int preverify_ok, X509_STORE_CTX* x509_ctx);
    int OnVerifyPeer(int preverify_ok, X509_STORE_CTX* x509_ctx);

    static void SrtpProfilesToString(etl::istring& output, const Options& options);

private:
    Dependencies _deps;
    const Options _options;

    SSL_CTX* _ctx = nullptr;
    SSL* _ssl = nullptr;
    BIO* _bio_read = nullptr;
    BIO* _bio_write = nullptr;

    State _state = State::kWaiting;

    Callback _send_callback;
    Callback _recv_callback;
    StateCallback _state_callback;
};

etl::string_stream& operator<<(etl::string_stream& ss, const Session::State& x);

}
