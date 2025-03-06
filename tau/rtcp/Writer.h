#pragma once

#include "tau/rtcp/Header.h"
#include "tau/memory/BufferView.h"

namespace rtcp {

// Writer is unsafe, need to control available size yourself 
class Writer {
public:
    explicit Writer(BufferView view);

    void WriteHeader(Type type, uint8_t rc, size_t length, bool padding = false);
    void Write(uint8_t value);
    void Write(uint16_t value);
    void Write(uint32_t value);
    void Write(uint64_t value);

    size_t GetSize() const { return _size; }
    size_t GetAvailableSize() const;

private:
    BufferView _view;
    size_t _size = 0;
};

}
