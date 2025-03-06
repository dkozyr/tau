#pragma once

#include "tau/rtcp/Header.h"
#include "tau/common/NetToHost.h"
#include <vector>

namespace rtcp {

class ByeReader {
public:
    static std::vector<uint32_t> GetSsrcs(const BufferViewConst& view) {
        const auto sc = GetRc(view.ptr[0]);
        std::vector<uint32_t> ssrcs(sc);
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
