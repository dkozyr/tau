#pragma once

#include "tau/rtcp/Header.h"
#include "tau/rtcp/NackMessage.h"
#include "tau/common/NetToHost.h"

namespace tau::rtcp {

class NackReader {
public:
    static uint32_t GetSenderSsrc(const BufferViewConst& view) {
        return Read32(view.ptr + kHeaderSize);
    }

    static uint32_t GetMediaSsrc(const BufferViewConst& view) {
        return Read32(view.ptr + kHeaderSize + sizeof(uint32_t));
    }

    static NackSns GetSns(const BufferViewConst& view) {
        NackSns sns;
        auto begin = view.ptr + kHeaderSize + 2 * sizeof(uint32_t);
        auto end = view.ptr + view.size;
        while(begin + sizeof(NackMessage) <= end) {
            auto pid = Read16(begin);
            auto blp = Read16(begin + sizeof(uint16_t));
            sns.insert(pid);
            while(blp != 0) {
                pid++;
                constexpr uint16_t kLeastBitMask = 1;
                if((blp & kLeastBitMask) == kLeastBitMask) {
                    sns.insert(pid);
                }
                blp >>= 1;
            }
            begin += sizeof(NackMessage);
        }
        return sns;
    }

    static bool Validate(const BufferViewConst& view) {
        return (view.size > kHeaderSize + 2 * sizeof(uint32_t)) && (view.size % sizeof(uint32_t) == 0);
    }
};

}
