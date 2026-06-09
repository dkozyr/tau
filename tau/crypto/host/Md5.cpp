#include "tau/crypto/Md5.h"
#include <openssl/hmac.h>

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
    const auto result = EVP_DigestUpdate(reinterpret_cast<EVP_MD_CTX*>(_ctx), view.data(), view.size());
    return (result == 1);
}

bool Md5Hasher::Finalize(uint8_t* output) {
    if(!_ctx) {
        return false;
    }
    uint32_t size = kMd5DigestLength;
    const auto result = EVP_DigestFinal_ex(reinterpret_cast<EVP_MD_CTX*>(_ctx), output, &size);
    return (result == 1);
}

bool Md5Hasher::Reset() {
    Deinit();
    return Init();
}

bool Md5Hasher::Init() {
    _ctx = EVP_MD_CTX_new();
    if(!_ctx) {
        return false;
    }

    const auto init = EVP_DigestInit_ex(reinterpret_cast<EVP_MD_CTX*>(_ctx), EVP_md5(), nullptr);
    if(init != 1) {
        _ctx = nullptr;
        return false;
    }
    return true;
}

void Md5Hasher::Deinit() {
    if(_ctx) {
        EVP_MD_CTX_free(reinterpret_cast<EVP_MD_CTX*>(_ctx));
        _ctx = nullptr;
    }
}

}
