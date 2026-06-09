#include "tau/crypto/Hmac.h"
#include <openssl/hmac.h>

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
    const auto result = EVP_DigestSignUpdate(reinterpret_cast<EVP_MD_CTX*>(_ctx), view.ptr, view.size);
    return (result == 1);
}

bool HmacHasher::Finalize(uint8_t* output) {
    if(!_ctx) {
        return false;
    }
    auto size = (_type == Type::Sha1) ? kHmacSha1Length : kHmacSha256Length;
    const auto result = EVP_DigestSignFinal(reinterpret_cast<EVP_MD_CTX*>(_ctx), output, &size);
    return (result == 1);
}

bool HmacHasher::Reset() {
    Deinit();
    return Init();
}

bool HmacHasher::Init() {
    _ctx = EVP_MD_CTX_new();
    if(!_ctx) {
        return false;
    }

    _pkey = EVP_PKEY_new_mac_key(EVP_PKEY_HMAC, nullptr, reinterpret_cast<const uint8_t*>(_password.data()), _password.size());
    if(!_pkey) {
        _ctx = nullptr;
        return false;
    }

    const auto init = EVP_DigestSignInit(reinterpret_cast<EVP_MD_CTX*>(_ctx), nullptr,
        (_type == Type::Sha1) ? EVP_sha1() : EVP_sha256(),
        nullptr, reinterpret_cast<EVP_PKEY*>(_pkey));
    if(init != 1) {
        _ctx = nullptr;
        _pkey = nullptr;
        return false;
    }
    return true;
}

void HmacHasher::Deinit() {
    if(_ctx) {
        EVP_MD_CTX_free(reinterpret_cast<EVP_MD_CTX*>(_ctx));
        _ctx = nullptr;
    }
    if(_pkey) {
        EVP_PKEY_free(reinterpret_cast<EVP_PKEY*>(_pkey));
        _pkey = nullptr;
    }
}

}
