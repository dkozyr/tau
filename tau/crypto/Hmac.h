#pragma once

#include "tau/memory/BufferView.h"
#include <etl/string.h>
#include <etl/string_view.h>

namespace tau::crypto {

inline constexpr size_t kHmacSha1Length = 20;
inline constexpr size_t kHmacSha256Length = 32;

class HmacHasher {
public:
    enum Type {
        Sha1,
        Sha256
    };

public:
    HmacHasher(Type type, const etl::string_view& password);
    ~HmacHasher();

    bool Update(const BufferViewConst& view);
    bool Finalize(uint8_t* output);
    bool Reset();

private:
    bool Init();
    void Deinit();

private:
    const Type _type;
    const etl::string<32> _password; //TODO: check capacity
    void* _ctx = nullptr;
    void* _pkey = nullptr;
};

}
