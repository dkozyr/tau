#pragma once

#include "tau/stun/Header.h"
#include "tau/stun/AttributeType.h"
#include "tau/memory/BufferView.h"
#include <string_view>

namespace tau::stun {

// Writer is unsafe, need to control available size yourself 
class Writer {
public:
    explicit Writer(BufferView view);

    void WriteHeader(uint16_t type);
    void WriteAttributeHeader(AttributeType type, size_t length);
    void UpdateHeaderLength();
    void SetHeaderLength(size_t length);
    void Write(uint8_t value);
    void Write(uint16_t value);
    void Write(uint32_t value);
    void Write(uint64_t value);
    void Write(std::string_view view);

    size_t GetSize() const { return _size; }
    size_t GetAvailableSize() const;
    BufferViewConst GetView() { return BufferViewConst{.ptr = _view.ptr, .size = _size}; }

private:
    BufferView _view;
    size_t _size = 0;
};

}
