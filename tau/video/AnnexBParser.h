#pragma once

#include <tau/video/AnnexB.h>
#include <tau/memory/BufferView.h>
#include <etl/vector.h>

namespace tau::video {

template<typename TBufferView>
size_t GetStartCodeLength(const TBufferView& view, size_t offset) {
    auto& data = view.ptr;
    if((offset + 3 <= view.size) && (data[offset] == 0) && (data[offset + 1] == 0)) {
        if(data[offset + 2] == 1) {
            return 3;
        }
        if((offset + 4 <= view.size) && (data[offset + 2] == 0) && (data[offset + 3] == 1)) {
            return 4;
        }
    }
    return 0;
}

// Fast SWAR check: returns non-zero if value contains at least one zero-byte
inline constexpr uint64_t HasZeroByte(uint64_t value) {
    return (value - 0x0101010101010101ULL) & (~value) & 0x8080808080808080ULL;
}

template<typename TBufferView>
size_t ParseAnnexB(const TBufferView& input, etl::ivector<TBufferView>& nal_units) {
    nal_units.clear();

    size_t current_nal_start = 0;
    size_t current_start_code_length = GetStartCodeLength(input, 0);
    if(current_start_code_length == 0) {
        return current_nal_start;
    }

    size_t offset = current_start_code_length;
    while(offset < input.size) {
        if(offset + sizeof(uint64_t) <= input.size) {
            auto words = reinterpret_cast<const uint64_t*>(input.ptr + offset);
            if(!HasZeroByte(*words)) {
                offset += sizeof(uint64_t);
                continue;
            }
        }

        auto code_length = GetStartCodeLength(input, offset);
        if(code_length > 0) {
            size_t nal_size = offset - current_nal_start;

            nal_units.push_back(TBufferView{
                .ptr = input.ptr + current_nal_start,
                .size = nal_size
            });
            if(nal_units.full()) {
                return offset;
            }

            current_nal_start = offset;
            offset += code_length;
        } else {
            offset++;
        }
    }

    if(current_nal_start + kAnnexB.size() <= input.size) {
        nal_units.push_back(TBufferView{
            .ptr = input.ptr + current_nal_start,
            .size = static_cast<size_t>(input.size - current_nal_start)
        });
    }

    return offset;
}

}
