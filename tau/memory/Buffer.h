#pragma once

#include "tau/memory/Allocator.h"
#include "tau/memory/BufferView.h"
#include <utility>

class Buffer {
public:
    static Buffer Create(Allocator& allocator, size_t size) {
        return std::move(Buffer(allocator, size));
    }

    Buffer(const Buffer&) = delete;
    Buffer(Buffer&&);
    ~Buffer();

    BufferView GetView();
    BufferViewConst GetView() const;

    BufferView GetViewWithCapacity();
    BufferViewConst GetViewWithCapacity() const;

    void SetSize(size_t size);

    size_t GetSize() const { return _size; }
    size_t GetCapacity() const { return _capacity; }

private:
    Buffer(Allocator& allocator, size_t size);

private:
    Allocator& _allocator;
    uint8_t* _block;
    size_t _capacity;
    size_t _size;
    size_t _offset = 0; //TODO: impl
};
