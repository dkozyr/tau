#include <tau/video/av1/Obu.h>
#include <tau/video/av1/Leb128.h>

namespace tau::av1 {

std::optional<Obu> ReadObu(BufferViewConst input, bool require_size) {
    Obu unit;
    if((input.size == 0) || (input.ptr[0] & (kObuForbiddenMask | kObuReservedMask))) {
        return std::nullopt;
    }
    const size_t header_size = (input.ptr[0] & kObuExtensionMask) ? 2 : 1;
    if((input.size < header_size) || ((header_size == 2) && (input.ptr[1] & kObuExtensionReservedMask))) {
        return std::nullopt;
    }
    const bool has_size = input.ptr[0] & kObuHasSizeMask;
    if(require_size && !has_size) {
        return std::nullopt;
    }
    unit.header = {input.ptr, header_size};
    const auto input_size = input.size;
    input.ForwardPtrUnsafe(header_size);

    auto payload_size = input.size;
    if(has_size) {
        const auto payload_size_opt = ReadLeb128(input);
        if(!payload_size_opt || (*payload_size_opt > input.size)) {
            return std::nullopt;
        }
        payload_size = *payload_size_opt;
    }
    unit.payload = {input.ptr, payload_size};
    unit.size = input_size - input.size + payload_size;
    return unit;
}

}
