#pragma once

#include "tau/memory/BufferView.h"
#include <etl/string_view.h>

namespace tau::crypto {

inline constexpr size_t kMd5DigestLength = 16;

class Md5Hasher {
public:
    Md5Hasher();
    ~Md5Hasher();

    bool Update(const etl::string_view& view);
    bool Finalize(uint8_t* output);
    bool Reset();

private:
    bool Init();
    void Deinit();

private:
    void* _ctx = nullptr;
};

}
