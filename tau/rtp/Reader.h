#pragma once

#include "tau/memory/BufferView.h"

namespace tau::rtp {

class Reader {
public:
    explicit Reader(BufferViewConst view);

    uint8_t Pt() const;
    uint32_t Ssrc() const;
    uint16_t Sn() const;
    uint32_t Ts() const;
    bool Marker() const;
    uint8_t Padding() const;

    BufferViewConst Payload() const;
    BufferViewConst Extensions() const;

    static bool Validate(BufferViewConst view);

private:
    static size_t GetExtensionLength(const BufferViewConst& view, size_t offset);

private:
    BufferViewConst _view;
};

}
