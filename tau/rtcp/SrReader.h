#pragma once

#include "tau/rtcp/Header.h"
#include "tau/rtcp/SrInfo.h"
#include "tau/rtcp/RrReader.h"
#include "tau/common/NetToHost.h"
#include <cassert>

namespace tau::rtcp {

class SrReader {
public:
    static uint32_t GetSenderSsrc(const BufferViewConst& view) {
        return Read32(view.ptr + kHeaderSize);
    }

    static SrInfo GetSrInfo(const BufferViewConst& view) {
        auto ptr = view.ptr + kHeaderSize + sizeof(uint32_t);
        return SrInfo{
            .ntp          = Read64(ptr),
            .ts           = Read32(ptr + 2 * sizeof(uint32_t)),
            .packet_count = Read32(ptr + 3 * sizeof(uint32_t)),
            .octet_count  = Read32(ptr + 4 * sizeof(uint32_t)),
        };
    }

    static RrBlocks GetBlocks(const BufferViewConst& view) {
        return RrReader::GetBlocks(view, sizeof(SrInfo));
    }

    static bool Validate(const BufferViewConst& view) {
        assert(view.size >= kHeaderSize); // validated by rtcp::Reader
        const auto blocks_count = GetRc(view.ptr[0]);
        const auto length = kHeaderSize + sizeof(uint32_t) + sizeof(SrInfo) + blocks_count * sizeof(RrBlock);
        return (view.size == length);
    }
};

}
