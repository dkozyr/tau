#pragma once

#include "tau/rtcp/Header.h"
#include "tau/common/NetToHost.h"
#include <etl/vector.h>
#include <algorithm>

namespace tau::rtcp {

class ByeReader {
public:
    static constexpr size_t kMaxSsrcs = 8;

public:
    static etl::vector<uint32_t, kMaxSsrcs> GetSsrcs(const BufferViewConst& view) {
        const auto sc = std::min<uint8_t>(kMaxSsrcs, GetRc(view.ptr[0]));
        etl::vector<uint32_t, 8> ssrcs(sc);
        auto begin = view.ptr + kHeaderSize;
        for(size_t i = 0; i < sc; ++i) {
            ssrcs[i] = Read32(begin);
            begin += sizeof(uint32_t);
        }
        return ssrcs;
    }

    static bool Validate(const BufferViewConst& view) {
        const auto sc = GetRc(view.ptr[0]);
        return view.size >= (kHeaderSize + sc * sizeof(uint32_t));
    }
};

}
