#pragma once

#include "tau/memory/Buffer.h"
#include <functional>

namespace tau::audio {

class EncoderInterface {
public:
    using Callback = std::function<void(Buffer&& encoded_frame)>;

public:
    virtual ~EncoderInterface() = default;

    virtual void SetCallback(Callback callback) = 0;
    virtual bool Encode(const Buffer& frame) = 0;
};

}
