#include "tau/crypto/Hmac.h"
#include "mbedtls/md.h"

namespace tau::crypto {

HmacHasher::HmacHasher(Type type, const etl::string_view& password)
    : _type(type)
    , _password(password) {
    Init();
}

HmacHasher::~HmacHasher() {
    Deinit();
}

bool HmacHasher::Update(const BufferViewConst& view) {
    if(!_ctx) {
        return false;
    }
    auto ctx = reinterpret_cast<mbedtls_md_context_t*>(_ctx);
    auto result = mbedtls_md_hmac_update(ctx, static_cast<const uint8_t*>(view.ptr), view.size);
    return (result == 0);
}

bool HmacHasher::Finalize(uint8_t* output) {
    if(!_ctx) {
        return false;
    }

    auto ctx = reinterpret_cast<mbedtls_md_context_t*>(_ctx);
    auto result = mbedtls_md_hmac_finish(ctx, output);
    return (result == 0);
}

bool HmacHasher::Reset() {
    auto ctx = reinterpret_cast<mbedtls_md_context_t*>(_ctx);
    auto result = mbedtls_md_hmac_reset(ctx);
    if(result != 0) {
        Deinit();
        return false;
    }
    return true;
}

bool HmacHasher::Init() {
    auto type = (_type == Type::Sha1) ? MBEDTLS_MD_SHA1 : MBEDTLS_MD_SHA256;
    const auto info = mbedtls_md_info_from_type(type);
    if(!info) {
        return false;
    }

    _ctx = malloc(sizeof(mbedtls_md_context_t));
    auto ctx = reinterpret_cast<mbedtls_md_context_t*>(_ctx);
    mbedtls_md_init(ctx);

    if(mbedtls_md_setup(ctx, info, 1) != 0) {
        Deinit();
        return false;
    }

    if(mbedtls_md_hmac_starts(ctx, reinterpret_cast<const uint8_t*>(_password.data()), _password.size()) != 0) {
        Deinit();
        return false;
    }
    return true;
}

void HmacHasher::Deinit() {
    if(_ctx) {
        auto ctx = reinterpret_cast<mbedtls_md_context_t*>(_ctx);
        mbedtls_md_free(ctx);
        free(_ctx);
        _ctx = nullptr;
    }
}

}
