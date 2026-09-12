#include <tau/video/av1/Leb128.h>

namespace tau::av1 {

std::optional<size_t> ReadLeb128(BufferViewConst& input) {
    size_t decoded = 0;
    for(size_t index = 0; (index < 8) && (input.size > 0); ++index) {
        const auto byte = input.ptr[0];
        input.ForwardPtrUnsafe(1);
        decoded |= static_cast<size_t>(byte & kLeb128ValueMask) << (index * 7);
        if(!(byte & kLeb128ContinuationMask)) {
            return decoded;
        }
    }
    return std::nullopt;
}

size_t WriteLeb128(uint8_t* output, size_t value) {
    size_t size = 0;
    do {
        output[size] = value & kLeb128ValueMask;
        value >>= 7;
        if(value) {
            output[size] |= kLeb128ContinuationMask;
        }
        ++size;
    } while(value);
    return size;
}

size_t Leb128Size(size_t value) {
    size_t size = 1;
    while(value >>= 7) {
        ++size;
    }
    return size;
}

}
