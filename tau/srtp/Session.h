#pragma once

#include "tau/memory/Buffer.h"
#include <srtp3/srtp.h>
#include <etl/vector.h>
#include <functional>

namespace tau::srtp {

class Session {
public:
    static constexpr auto kKeyCapacity = SRTP_MAX_KEY_LEN;
    static constexpr auto kSaltCapacity = SRTP_SALT_LEN;
    static constexpr auto kRtxWindowSize = 256;

    enum Type {
        kEncryptor,
        kDecryptor,
    };

    struct Options {
        Type type;
        srtp_profile_t profile;
        etl::vector<uint8_t, kKeyCapacity> key;
        etl::vector<uint8_t, kSaltCapacity> salt;
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

    static size_t GetKeySize(srtp_profile_t profile);
    static size_t GetSaltSize(srtp_profile_t profile);

private:
    const etl::string_view _log_ctx;
    srtp_t _session = nullptr;
    Callback _callback;
};

}
