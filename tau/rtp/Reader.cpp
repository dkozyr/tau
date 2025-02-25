#include <tau/rtp/Reader.h>
#include <tau/rtp/Constants.h>
#include <tau/rtp/details/FixedHeader.h>
#include <tau/common/NetToHost.h>
#include <algorithm>

namespace rtp {

using namespace detail;

Reader::Reader(BufferViewConst view)
    : _view(std::move(view)) {
}

uint8_t Reader::Pt() const {
    return (_view.ptr[1] & 0x7F);
}

uint32_t Reader::Ssrc() const {
    return Read32(_view.ptr + 2 * sizeof(uint32_t));
}

uint16_t Reader::Sn() const {
    return Read16(_view.ptr + sizeof(uint16_t));
}

uint32_t Reader::Ts() const {
    return Read32(_view.ptr + sizeof(uint32_t));
}

bool Reader::Marker() const {
    return (_view.ptr[1] & 0x80);
}

uint8_t Reader::Padding() const {
    auto header = reinterpret_cast<const FixedHeader*>(&_view.ptr[0]);
    return header->p ? std::max<uint8_t>(1, _view.ptr[_view.size - 1]) : 0;
}

BufferViewConst Reader::Payload() const {
    auto header = reinterpret_cast<const FixedHeader*>(&_view.ptr[0]);
    auto offset = kFixedHeaderSize + header->cc * sizeof(uint32_t);
    if(header->x) {
        offset += GetExtensionLength(_view, offset);
    }
    const auto begin = _view.ptr + offset;
    const auto end = _view.ptr + _view.size - Padding();
    return BufferViewConst{
        .ptr = begin,
        .size = static_cast<size_t>(end - begin)
    };
}

BufferViewConst Reader::Extensions() const {
    auto header = reinterpret_cast<const FixedHeader*>(&_view.ptr[0]);
    if(!header->x) {
        return BufferViewConst{
            .ptr = nullptr,
            .size = 0
        };
    }

    const auto offset = kFixedHeaderSize + header->cc * sizeof(uint32_t);
    const auto begin = _view.ptr + offset;
    const auto end = begin + GetExtensionLength(_view, offset);
    return BufferViewConst{
        .ptr = begin,
        .size = static_cast<size_t>(end - begin)
    };
}

bool Reader::Validate(BufferViewConst view) {
    if(view.size < kFixedHeaderSize) {
        return false;
    }

    auto header = reinterpret_cast<const FixedHeader*>(&view.ptr[0]);
    if(header->v != 2) {
        return false;
    }

    size_t min_size = kFixedHeaderSize + header->cc * sizeof(uint32_t);
    if(header->x) {
        if(view.size < min_size + kExtensionHeaderSize) {
            return false;
        }
        min_size += GetExtensionLength(view, min_size);
    }

    if(header->p) {
        if(view.size <= min_size) {
            return false;
        }
        min_size += std::max<uint8_t>(1, view.ptr[view.size - 1]);
    }

    return view.size >= min_size;
}

size_t Reader::GetExtensionLength(const BufferViewConst& view, size_t offset) {
    const auto extension_length_in_words = Read16(view.ptr + offset + sizeof(uint16_t));
    return HeaderExtensionSize(extension_length_in_words);
}

}
