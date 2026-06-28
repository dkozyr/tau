#include "tau/memory/Buffer.h"
#include "tau/common/Base64.h"
#include "tau/common/Math.h"
#include "tau/common/Exception.h"

namespace tau {

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

Buffer::Buffer(Allocator& allocator)
    : _allocator(allocator)
    , _block(nullptr)
    , _capacity(0)
    , _size(0)
    , _offset(0)
    , _info({})
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
        TAU_EXCEPTION(std::runtime_error, "Cannot move-assign with different Allocator");
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

Buffer Buffer::Create(Allocator& allocator, const BufferViewConst& view, Info info) {
    Buffer buffer(allocator, view.size, info);
    memcpy(buffer.GetView().ptr, view.ptr, view.size);
    buffer.SetSize(view.size);
    return buffer;
}

Buffer Buffer::MakeCopy() const {
    Buffer buffer_copy(_allocator, _capacity, _info);
    buffer_copy.SetSize(_size);
    memcpy(buffer_copy.GetView().ptr, _block, _size);
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

const etl::string_view Buffer::GetStringView() const {
    return etl::string_view{reinterpret_cast<const char*>(_block), _size};
}

void Buffer::SetSize(size_t size) {
    if(size > _capacity) {
        //TODO: do exception?
        size = _capacity;
    }
    _size = size;
}

Buffer CreateBufferFromBase64(Allocator& allocator, etl::string_view str, Buffer::Info info) {
    constexpr size_t kMaxOutputCapacity = 1024;
    const auto expected_size = DivCeil(str.size() * 6, 8);
    if(expected_size > kMaxOutputCapacity) {
        return Buffer::Create(allocator, 0, {});
    }

    etl::string<kMaxOutputCapacity> decoded;
    Base64Decode(str, decoded);
    auto buffer = Buffer::Create(allocator, expected_size, info);
    memcpy(buffer.GetView().ptr, decoded.data(), decoded.size());
    buffer.SetSize(decoded.size());
    return buffer;
}

}
