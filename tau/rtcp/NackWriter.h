#pragma once

#include "tau/rtcp/Writer.h"
#include "tau/rtcp/NackMessage.h"
#include "tau/rtp/Sn.h"

namespace rtcp {

class NackWriter {
public:
    static constexpr auto kBlpBitCount = 8 * sizeof(uint16_t);

public:
    static bool Write(Writer& writer, uint32_t sender_ssrc, uint32_t media_ssrc, const NackSns& sns) {
        const auto length = kHeaderSize + sizeof(sender_ssrc) + sizeof(media_ssrc) + CalcNackMessagesCount(sns) * sizeof(NackMessage);
        if(sns.empty() || (writer.GetAvailableSize() < length)) {
            return false;
        }

        writer.WriteHeader(Type::kRtpfb, RtpfbType::kNack, length);
        writer.Write(sender_ssrc);
        writer.Write(media_ssrc);

        uint16_t pid = *sns.begin();
        uint16_t blp = 0;
        writer.Write(pid);

        for(auto it = std::next(sns.begin()); it != sns.end(); it++) {
            const auto delta = rtp::SnDelta(*it, pid);
            if(delta <= kBlpBitCount) {
                blp |= (1u << (delta - 1));
            } else {
                writer.Write(blp);    
                blp = 0;

                pid = *it;
                writer.Write(pid);
            }
        }
        writer.Write(blp);
        return true;
    }

private:
    static size_t CalcNackMessagesCount(const NackSns& sns) {
        size_t count = 0;
        uint16_t pid = sns.empty() ? 0 : rtp::SnBackward(*sns.begin(), kBlpBitCount + 1);
        for(auto sn : sns) {
            if(rtp::SnDelta(sn, pid) > kBlpBitCount) {
                pid = sn;
                count++;
            }
        }
        return count;
    }
};

}
