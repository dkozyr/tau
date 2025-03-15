#pragma once

#include "tau/rtcp/Header.h"
#include "tau/common/NetToHost.h"

namespace tau::rtcp {

class FirReader {
public:
    static uint32_t GetSenderSsrc(const BufferViewConst& view) {
        return Read32(view.ptr + kHeaderSize);
    }

    static uint32_t GetMediaSsrc(const BufferViewConst& view) {
        return Read32(view.ptr + kHeaderSize + sizeof(uint32_t));
    }

    static uint8_t GetSn(const BufferViewConst& view) {
        return view.ptr[kHeaderSize + 2 * sizeof(uint32_t)];
    }

    static bool Validate(const BufferViewConst& view) {
        return view.size == kHeaderSize + 3 * sizeof(uint32_t);
    }
};

}
