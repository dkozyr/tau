#pragma once

#include "tau/stun/Header.h"
#include "tau/stun/AttributeType.h"
#include "tau/memory/BufferView.h"
#include <functional>

namespace tau::stun {

class Reader {
public:
    using AttributeCallback = std::function<bool(AttributeType, const BufferViewConst&)>;

public:
    static bool ForEachAttribute(const BufferViewConst& view, AttributeCallback callback);
    static bool Validate(const BufferViewConst& view);
};

}
