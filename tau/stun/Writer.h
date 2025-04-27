#pragma once

#include "tau/stun/Header.h"
#include "tau/stun/AttributeType.h"
#include "tau/memory/Writer.h"

namespace tau::stun {

// Writer is unsafe, need to control available size yourself 
class Writer {
public:
    Writer(BufferView view, uint16_t type);

    void WriteAttributeHeader(AttributeType type, size_t length);
    void UpdateHeaderLength();
    void SetHeaderLength(size_t length);
    void Write(uint8_t value);
    void Write(uint16_t value);
    void Write(uint32_t value);
    void Write(uint64_t value);
    void Write(std::string_view view);

    size_t GetSize() const;
    size_t GetAvailableSize() const;
    BufferViewConst GetView();

private:
    tau::Writer _writer;
    BufferView _view;
};

}
