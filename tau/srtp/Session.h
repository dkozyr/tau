#pragma once

#include <tau/srtp/KeyMaterial.h>
#include <tau/memory/Buffer.h>
#include <functional>

namespace tau::srtp {

class Session {
public:
    static constexpr auto kRtxWindowSize = 256;

    enum Type {
        kEncryptor,
        kDecryptor,
    };

    struct Options {
        Type type;
        srtp_profile_t profile;
        KeyMaterial key_material;
        etl::string_view log_ctx;
    };

    using Callback = std::function<void(Buffer&& decrypted, bool is_rtp)>;

public:
    explicit Session(Options&& options);
    ~Session();

    bool IsValid() const;
    void SetCallback(Callback callback) { _callback = std::move(callback); }

    bool Encrypt(Buffer&& packet, bool is_rtp = true);
    bool Decrypt(Buffer&& packet, bool is_rtp = true);

private:
    const etl::string_view _log_ctx;
    srtp_t _session = nullptr;
    Callback _callback;
};

}
