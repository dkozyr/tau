#pragma once

#include <etl/string.h>

namespace tau::mdns {

using Name = etl::string<36 + 1 + 5>; // UUID + ".local"

}
