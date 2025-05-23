#pragma once

#include "tau/common/Variant.h"

namespace tau::webrtc {

struct EventPli{};
struct EventFir{};

using Event = std::variant<EventPli, EventFir>;

}
