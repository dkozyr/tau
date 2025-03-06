#pragma once

#include "tau/rtcp/Writer.h"

namespace rtcp {

class FirWriter {
public:
    static bool Write(Writer& writer, uint32_t sender_ssrc, uint32_t media_ssrc, uint8_t sn) {
        const auto length = kHeaderSize + sizeof(sender_ssrc) + sizeof(media_ssrc) + sizeof(uint32_t);
        if(writer.GetAvailableSize() < length) {
            return false;
        }

        writer.WriteHeader(Type::kPsfb, PsfbType::kFir, length);
        writer.Write(sender_ssrc);
        writer.Write(media_ssrc);
        writer.Write(static_cast<uint32_t>(sn) << 24);
        return true;
    }
};

}
