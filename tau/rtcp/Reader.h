#pragma once

#include "tau/rtcp/Header.h"
#include <functional>

namespace tau::rtcp {

class Reader {
public:
    using ReportCallback = std::function<bool(Type type, const BufferViewConst& report)>;

public:
    static bool ForEachReport(const BufferViewConst& view, ReportCallback callback);
    static bool Validate(const BufferViewConst& view);
};

}
