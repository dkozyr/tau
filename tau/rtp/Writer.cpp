#include "tau/rtp/Writer.h"
#include "tau/rtp/Constants.h"
#include "tau/rtp/details/FixedHeader.h"
#include "tau/common/NetToHost.h"
#include <cstring>

namespace tau::rtp {

Writer::Result Writer::Write(BufferView view_with_capacity, const Options& options) {
    const auto extension_size = HeaderExtensionSize(options.extension_length_in_words);
    Result result = {
        .size = 3 * sizeof(uint32_t) + extension_size
    };
    if(view_with_capacity.size < result.size) {
        result.size = 0;
        return result;
    }

    auto ptr = view_with_capacity.ptr;
    ptr[0] = detail::BuildFixedHeader(options.extension_length_in_words > 0);
    ptr[1] = (options.pt & 0x7F) | (options.marker ? 0x80 : 0);
    Write16(ptr + 2, options.sn);
    Write32(ptr + 4, options.ts);
    Write32(ptr + 8, options.ssrc);
    if(options.extension_length_in_words) {
        result.extension = BufferView{
            .ptr = ptr + kFixedHeaderSize + kExtensionHeaderSize,
            .size = extension_size - kExtensionHeaderSize
        };
        Write16(ptr + kFixedHeaderSize, 0xBEDE);
        Write16(ptr + kFixedHeaderSize + sizeof(uint16_t), options.extension_length_in_words);
        std::memset(result.extension.ptr, 0, result.extension.size);
    }
    result.payload = BufferView{
        .ptr = ptr + kFixedHeaderSize + extension_size,
        .size = 0
    };
    return result;
}

bool Writer::AddPadding(BufferView, uint8_t) {
    return false; //TODO: implement
}

}
