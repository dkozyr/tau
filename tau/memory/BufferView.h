#pragma once

#include <cstddef>
#include <cstdint>

struct BufferView {
    uint8_t* ptr;
    size_t size;

    void ForwardPtrUnsafe(size_t offset) {
        ptr += offset;
        size -= offset;
    }
};

struct BufferViewConst {
    const uint8_t* ptr;
    size_t size;

    void ForwardPtrUnsafe(size_t offset) {
        ptr += offset;
        size -= offset;
    }
};

inline constexpr auto kBufferViewNull = BufferView{.ptr = nullptr, .size = 0};

inline BufferViewConst ToConst(BufferView view) {
    return BufferViewConst{
        .ptr = view.ptr,
        .size = view.size
    };
}
