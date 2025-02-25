#include "tau/memory/Buffer.h"

Buffer::Buffer(Allocator& allocator, size_t size)
    : _allocator(allocator)
    , _block(_allocator.Allocate(size))
    , _capacity(size)
    , _size(0)
    , _offset(0)
{}

Buffer::Buffer(Buffer&& other)
    : _allocator(other._allocator)
    , _block(other._block)
    , _capacity(other._capacity)
    , _size(other._size)
    , _offset(other._offset) {
    other._block = nullptr;
    other._capacity = 0;
    other._size = 0;
    other._offset = 0;
}

Buffer::~Buffer() {
    _allocator.Deallocate(_block);
}

BufferView Buffer::GetView() {
    return BufferView{
        .ptr = _block,
        .size = _size
    };
}

BufferViewConst Buffer::GetView() const {
    return BufferViewConst{
        .ptr = _block,
        .size = _size
    };
}

BufferView Buffer::GetViewWithCapacity() {
    return BufferView{
        .ptr = _block,
        .size = _capacity
    };
}

BufferViewConst Buffer::GetViewWithCapacity() const {
    return BufferViewConst{
        .ptr = _block,
        .size = _capacity
    };
}

void Buffer::SetSize(size_t size) {
    if(size > _capacity) {
        //TODO: do exception?
        size = _capacity;
    }
    _size = size;
}
