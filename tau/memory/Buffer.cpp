#include "tau/memory/Buffer.h"
#include <cstring>

Buffer::Buffer(Allocator& allocator, size_t capacity, Info info)
    : _allocator(allocator)
    , _block(_allocator.Allocate(capacity))
    , _capacity(capacity)
    , _size(0)
    , _offset(0)
    , _info(info)
{}

Buffer::Buffer(Allocator& allocator, Info info)
    : _allocator(allocator)
    , _block(_allocator.Allocate())
    , _capacity(_allocator.GetChunkSize())
    , _size(0)
    , _offset(0)
    , _info(info)
{}

Buffer::Buffer(Buffer&& other)
    : _allocator(other._allocator)
    , _block(other._block)
    , _capacity(other._capacity)
    , _size(other._size)
    , _offset(other._offset)
    , _info(other._info) {
    other._block = nullptr;
}

Buffer& Buffer::operator=(Buffer&& other) {
    if(&_allocator != &other._allocator) {
        throw -1;
    }
    _block = other._block;
    _capacity = other._capacity;
    _size = other._size;
    _offset = other._offset;
    _info = std::move(other._info);

    other._block = nullptr;
    return *this;
}

Buffer::~Buffer() {
    if(_block) {
        _allocator.Deallocate(_block);
    }
}

Buffer Buffer::MakeCopy() const {
    Buffer buffer_copy(_allocator, _capacity, _info);
    buffer_copy.SetSize(_size);
    std::memcpy(buffer_copy.GetView().ptr, _block, _size);
    return buffer_copy;
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
