#pragma once

#include "tau/rtcp/Header.h"
#include "tau/rtcp/RrBlock.h"
#include "tau/common/NetToHost.h"
#include <cassert>

namespace rtcp {

class RrReader {
public:
    static uint32_t GetSenderSsrc(const BufferViewConst& view) {
        return Read32(view.ptr + kHeaderSize);
    }

    static RrBlocks GetBlocks(const BufferViewConst& view) {
        RrBlocks blocks;
        auto blocks_count = GetRc(view.ptr[0]);
        auto ptr = view.ptr + kHeaderSize + sizeof(uint32_t);
        while(blocks_count != 0) {
            blocks.push_back(RrBlock{
                .ssrc            = Read32(ptr),
                .fraction_lost   = ptr[sizeof(uint32_t)],
                .cumulative_lost = {
                    ptr[sizeof(uint32_t) + 3],
                    ptr[sizeof(uint32_t) + 2],
                    ptr[sizeof(uint32_t) + 1]
                },
                .ext_highest_sn  = Read32(ptr + 2 * sizeof(uint32_t)),
                .jitter          = Read32(ptr + 3 * sizeof(uint32_t)),
                .lsr             = Read32(ptr + 4 * sizeof(uint32_t)),
                .dlsr            = Read32(ptr + 5 * sizeof(uint32_t))
            });
            blocks_count--;
            ptr += sizeof(RrBlock);
        }
        return blocks;
    }

    static bool Validate(const BufferViewConst& view) {
        assert(view.size >= kHeaderSize); // validated by rtcp::Reader
        const auto blocks_count = GetRc(view.ptr[0]);
        const auto length = kHeaderSize + sizeof(uint32_t) + blocks_count * sizeof(RrBlock);
        return (view.size == length);
    }
};

}
