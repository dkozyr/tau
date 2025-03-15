#pragma once

#include "tau/rtcp/Writer.h"

namespace tau::rtcp {

class PliWriter {
public:
    static bool Write(Writer& writer, uint32_t sender_ssrc, uint32_t media_ssrc) {
        const auto length = kHeaderSize + sizeof(sender_ssrc) + sizeof(media_ssrc);
        if(writer.GetAvailableSize() < length) {
            return false;
        }

        writer.WriteHeader(Type::kPsfb, PsfbType::kPli, length);
        writer.Write(sender_ssrc);
        writer.Write(media_ssrc);
        return true;
    }
};

}
