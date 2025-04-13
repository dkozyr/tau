#pragma once

#include "tau/dtls/Certificate.h"
#include "tau/memory/Buffer.h"
#include "tau/common/Clock.h"
#include <openssl/ssl.h>
#include <vector>
#include <functional>
#include <string_view>

namespace tau::dtls {

class Session {
    using BioData = std::vector<uint8_t>;

public:
    enum Type {
        kServer,
        kClient
    };

    enum State {
        kWaiting,
        kConnecting,
        kConnected,
        kFailed //TODO: impl
    };

    struct Dependencies {
        Clock& clock;
        Allocator& udp_allocator;
        Certificate& certificate;
    };

    struct Options{
        Type type;
        std::string remote_peer_cert_digest;
        std::string log_ctx;
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

private:
    static int OnVerifyPeerStatic(int preverify_ok, X509_STORE_CTX* x509_ctx);
    int OnVerifyPeer(int preverify_ok, X509_STORE_CTX* x509_ctx);

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

}
