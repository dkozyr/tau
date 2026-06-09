#pragma once

#include "tau/memory/Buffer.h"
#include <etl/vector.h>

namespace tau::rtp {

using Frame = etl::vector<Buffer, 128>; //TODO: check capacity

}
