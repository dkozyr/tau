#pragma once

#include "tau/memory/Buffer.h"
#include <functional>

namespace tau::audio {

class DecoderInterface {
public:
    static constexpr size_t kDefaultFrameDurationMs = 20;

    using Callback = std::function<void(Buffer&& decoded_frame)>;

public:
    virtual ~DecoderInterface() = default;

    virtual void SetCallback(Callback callback) = 0;
    virtual bool Decode(Buffer&& frame) = 0;
    virtual bool Decode(const BufferViewConst& frame_view, Timepoint tp) = 0;
    virtual bool DecodePlc(Timepoint tp) = 0;
};

}
