#pragma once

#include <etl/string.h>
#include <variant>

namespace tau::ws {

using String = etl::string<8192>; //TODO: memory pool?
struct CloseMessage{};
struct DoNothingMessage{};

using Message = std::variant<String, CloseMessage, DoNothingMessage>;

}
