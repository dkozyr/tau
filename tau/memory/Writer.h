#pragma once

#include "tau/memory/BufferView.h"
#include <string_view>

namespace tau {

// Writer is unsafe, need to control available size yourself 
class Writer {
public:
    Writer(BufferView view);

    void Write(uint8_t value);
    void Write(uint16_t value);
    void Write(uint32_t value);
    void Write(uint64_t value);
    void Write(std::string_view view);
    void MoveForward(size_t offset);

    size_t GetSize() const { return _size; }
    size_t GetAvailableSize() const;
    BufferViewConst GetView() { return BufferViewConst{.ptr = _view.ptr, .size = _size}; }

private:
    BufferView _view;
    size_t _size = 0;
};

}
