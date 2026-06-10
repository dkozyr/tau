#pragma once

#include "tau/rtcp/Writer.h"
#include <etl/vector.h>

namespace tau::rtcp {

class ByeWriter {
public:
    static bool Write(Writer& writer, const etl::ivector<uint32_t>& ssrcs) {
        const auto length = kHeaderSize + ssrcs.size() * sizeof(uint32_t);
        if(writer.GetAvailableSize() < length) {
            return false;
        }

        writer.WriteHeader(Type::kBye, ssrcs.size(), length);
        for(auto ssrc : ssrcs) {
            writer.Write(ssrc);
        }
        return true;
    }
};

}
