#pragma once

#include "tau/rtcp/Header.h"
#include "tau/rtcp/RrBlock.h"
#include "tau/common/NetToHost.h"
#include <cassert>

namespace tau::rtcp {

class RrReader {
public:
    static uint32_t GetSenderSsrc(const BufferViewConst& view) {
        return Read32(view.ptr + kHeaderSize);
    }

    static RrBlocks GetBlocks(const BufferViewConst& view, size_t rr_blocks_offset = 0) {
        RrBlocks blocks;
        auto blocks_count = GetRc(view.ptr[0]);
        auto ptr = view.ptr + kHeaderSize + sizeof(uint32_t) + rr_blocks_offset;
        while(blocks_count != 0) {
            blocks.push_back(RrBlock{
                .ssrc             = Read32(ptr),
                .packet_lost_word = Read32(ptr + sizeof(uint32_t)),
                .ext_highest_sn   = Read32(ptr + 2 * sizeof(uint32_t)),
                .jitter           = Read32(ptr + 3 * sizeof(uint32_t)),
                .lsr              = Read32(ptr + 4 * sizeof(uint32_t)),
                .dlsr             = Read32(ptr + 5 * sizeof(uint32_t))
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
