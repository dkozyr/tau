#include "tau/rtcp/Writer.h"
#include "tau/common/NetToHost.h"
#include <cassert>

namespace rtcp {

Writer::Writer(BufferView view)
    : _view(view)
{}

void Writer::WriteHeader(Type type, uint8_t rc, size_t length, bool padding) {
    assert(_size + kHeaderSize <= _view.size);
    assert(length > 0);
    _view.ptr[_size] = BuildHeader(rc, padding);
    _view.ptr[_size + 1] = type;
    Write16(_view.ptr + _size + 2, LengthToWordsMinusOne(length));
    _size += kHeaderSize;
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

size_t Writer::GetAvailableSize() const {
    return (_view.size > _size) ? (_view.size - _size) : 0;
}

}
