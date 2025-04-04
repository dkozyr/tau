#pragma once

#include "tau/stun/Writer.h"

namespace tau::stun::attribute {

class DataReader {
public:
    static BufferViewConst GetData(const BufferViewConst& view);
};

class DataWriter {
public:
    static bool Write(Writer& writer, const BufferViewConst& data);
};

}
