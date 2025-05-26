#pragma once

#include <string>
#include <variant>

namespace tau::ws {

struct CloseMessage{};
struct DoNothingMessage{};
using Message = std::variant<std::string, CloseMessage, DoNothingMessage>;

}
