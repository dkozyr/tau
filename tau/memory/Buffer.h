#pragma once

#include "tau/memory/Allocator.h"
#include "tau/memory/BufferView.h"
#include "tau/common/Clock.h"
#include <utility>

namespace tau {

class Buffer {
public:
    struct Info {
        Timepoint tp = 0;

        bool operator==(const Info& other) const {
            return tp == other.tp;
        }
    };

public:
    static Buffer Create(Allocator& allocator, size_t capacity, Info info = Info{.tp = 0}) {
        return std::move(Buffer(allocator, capacity, info));
    }

    static Buffer Create(Allocator& allocator, Info info = Info{.tp = 0}) {
        return std::move(Buffer(allocator, info));
    }

    Buffer(const Buffer&) = delete;
    Buffer(Buffer&&);
    Buffer& operator=(const Buffer& other) = delete;
    Buffer& operator=(Buffer&& other);
    ~Buffer();

    Buffer MakeCopy() const;

    BufferView GetView();
    BufferViewConst GetView() const;

    BufferView GetViewWithCapacity();
    BufferViewConst GetViewWithCapacity() const;

    void SetSize(size_t size);

    size_t GetSize() const { return _size; }
    size_t GetCapacity() const { return _capacity; }

    Info& GetInfo() { return _info; }
    const Info& GetInfo() const { return _info; }

private:
    Buffer(Allocator& allocator, size_t capacity, Info info);
    Buffer(Allocator& allocator, Info info);

private:
    Allocator& _allocator;
    uint8_t* _block;
    size_t _capacity;
    size_t _size;
    size_t _offset = 0; //TODO: impl
    Info _info;
};

}
