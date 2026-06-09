#pragma once

#include "tau/sdp/line/LineType.h"
#include <etl/string_view.h>
#include <optional>
#include <functional>
#include <cstddef>

namespace tau::sdp {

class Reader {
public:
    using LineCallback = std::function<bool(std::optional<size_t> media_index, LineType type, etl::string_view value)>;

public:
    static bool ForEachLine(const etl::string_view& sdp, LineCallback callback);

    static bool Validate(const etl::string_view& sdp);

private:
    static std::optional<LineType> ParseType(char c);
};

}
