#include "tau/mdns/Writer.h"
#include "tau/common/NetToHost.h"
#include <cstring>
#include <cassert>

namespace tau::mdns {

Writer::Writer(BufferView view) : _view(view) {
}

void Writer::Write(uint8_t value) {
    assert(_size + sizeof(uint8_t) <= _view.size);
    _view.ptr[_size] = value;
    _size += sizeof(uint8_t);
}

void Writer::Write(uint16_t value) {
    assert(_size + sizeof(uint16_t) <= _view.size);
    Write16(_view.ptr + _size, value);
    _size += sizeof(uint16_t);
}

void Writer::Write(uint32_t value) {
    assert(_size + sizeof(uint32_t) <= _view.size);
    Write32(_view.ptr + _size, value);
    _size += sizeof(uint32_t);
}

void Writer::Write(uint64_t value) {
    assert(_size + sizeof(uint64_t) <= _view.size);
    Write64(_view.ptr + _size, value);
    _size += sizeof(uint64_t);
}

void Writer::Write(std::string_view view) {
    assert(_size + view.size() <= _view.size);
    std::memcpy(_view.ptr + _size, view.data(), view.size());
    _size += view.size();
}

size_t Writer::GetAvailableSize() const {
    return (_view.size > _size) ? (_view.size - _size) : 0;
}

}
