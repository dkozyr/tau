#include "tau/stun/Writer.h"
#include "tau/stun/MagicCookie.h"
#include "tau/common/NetToHost.h"
#include <cassert>

namespace tau::stun {

Writer::Writer(BufferView view, uint16_t type) : _writer(view), _view(view) {
    assert(kMessageHeaderSize <= _view.size);
    _writer.Write(type);
    _writer.Write(static_cast<uint16_t>(0));
    _writer.Write(kMagicCookie);
    _writer.MoveForward(kTransactionIdSize);
}

void Writer::WriteAttributeHeader(AttributeType type, size_t length) {
    _writer.Write(static_cast<uint16_t>(type));
    _writer.Write(static_cast<uint16_t>(length));
}

void Writer::UpdateHeaderLength() {
    Write16(_view.ptr + 2, _writer.GetSize() - kMessageHeaderSize);
}

void Writer::SetHeaderLength(size_t length) {
    Write16(_view.ptr + 2, length);
}

void Writer::Write(uint8_t value) {
    _writer.Write(value);
}

void Writer::Write(uint16_t value) {
    _writer.Write(value);
}

void Writer::Write(uint32_t value) {
    _writer.Write(value);
}

void Writer::Write(uint64_t value) {
    _writer.Write(value);
}

void Writer::Write(std::string_view value) {
    _writer.Write(value);
}

size_t Writer::GetSize() const {
    return _writer.GetSize();
}

size_t Writer::GetAvailableSize() const {
    return _writer.GetAvailableSize();
}

BufferViewConst Writer::GetView() {
    return _writer.GetView();
}

}
