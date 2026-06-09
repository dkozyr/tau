#include "tau/crypto/Md5.h"
#include "mbedtls/md5.h"

namespace tau::crypto {

Md5Hasher::Md5Hasher() {
    Init();
}

Md5Hasher::~Md5Hasher() {
    Deinit();
}

bool Md5Hasher::Update(const etl::string_view& view) {
    if(!_ctx) {
        return false;
    }
    auto ctx = reinterpret_cast<mbedtls_md5_context*>(_ctx);
    auto result = mbedtls_md5_update(ctx, reinterpret_cast<const uint8_t*>(view.data()), view.size());
    return (result == 0);
}

bool Md5Hasher::Finalize(uint8_t* output) {
    if(!_ctx) {
        return false;
    }

    auto ctx = reinterpret_cast<mbedtls_md5_context*>(_ctx);
    auto result = mbedtls_md5_finish(ctx, output);
    return (result == 0);
}

bool Md5Hasher::Reset() {
    auto ctx = reinterpret_cast<mbedtls_md5_context*>(_ctx);
    auto result = mbedtls_md5_starts(ctx);
    if(result != 0) {
        Deinit();
        return false;
    }
    return true;
}

bool Md5Hasher::Init() {
    _ctx = malloc(sizeof(mbedtls_md5_context));
    auto ctx = reinterpret_cast<mbedtls_md5_context*>(_ctx);
    mbedtls_md5_init(ctx);

    if(mbedtls_md5_starts(ctx) != 0) {
        Deinit();
        return false;
    }
    return true;
}

void Md5Hasher::Deinit() {
    if(_ctx) {
        auto ctx = reinterpret_cast<mbedtls_md5_context*>(_ctx);
        mbedtls_md5_free(ctx);
        free(_ctx);
        _ctx = nullptr;
    }
}

}
