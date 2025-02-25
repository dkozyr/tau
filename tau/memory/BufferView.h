#pragma once

#include <cstddef>
#include <cstdint>

struct BufferView {
    uint8_t* ptr;
    size_t size;
};

struct BufferViewConst {
    const uint8_t* ptr;
    size_t size;
};

inline constexpr auto kBufferViewNull = BufferView{.ptr = nullptr, .size = 0};

inline BufferViewConst ToConst(BufferView view) {
    return BufferViewConst{
        .ptr = view.ptr,
        .size = view.size
    };
}
