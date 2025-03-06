#pragma once

#include "tau/rtcp/Writer.h"
#include "tau/rtcp/RrBlock.h"

namespace rtcp {

class RrWriter {
public:
    static bool Write(Writer& writer, uint32_t sender_ssrc, const RrBlocks& blocks) {
        const auto length = kHeaderSize + sizeof(sender_ssrc) + blocks.size() * sizeof(RrBlock);
        if(writer.GetAvailableSize() < length) {
            return false;
        }

        writer.WriteHeader(Type::kRr, blocks.size(), length);
        writer.Write(sender_ssrc);
        for(auto& block : blocks) {
            writer.Write(block.ssrc);
            writer.Write(block.fraction_lost);
            writer.Write(block.cumulative_lost[2]);
            writer.Write(block.cumulative_lost[1]);
            writer.Write(block.cumulative_lost[0]);
            writer.Write(block.ext_highest_sn);
            writer.Write(block.jitter);
            writer.Write(block.lsr);
            writer.Write(block.dlsr);
        }
        return true;
    }
};

}
