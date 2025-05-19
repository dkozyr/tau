#pragma once

#include "tau/memory/Buffer.h"
#include <srtp3/srtp.h>
#include <functional>
#include <vector>

namespace tau::srtp {

class Session {
public:
    static constexpr auto kRtxWindowSize = 1024;

    enum Type {
        kEncryptor,
        kDecryptor,
    };

    struct Options {
        Type type;
        srtp_profile_t profile;
        std::vector<uint8_t> key;
        std::string_view log_ctx;
    };

    using Callback = std::function<void(Buffer&& decrypted, bool is_rtp)>;

public:
    explicit Session(Options&& options);
    ~Session();

    void SetCallback(Callback callback) { _callback = std::move(callback); }

    bool Encrypt(Buffer&& packet, bool is_rtp = true);
    bool Decrypt(Buffer&& packet, bool is_rtp = true);

private:
    const std::string_view _log_ctx;
    srtp_t _session;
    Callback _callback;
};

}
