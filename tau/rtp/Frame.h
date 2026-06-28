#pragma once

#include "tau/memory/Buffer.h"
#include <etl/vector.h>

namespace tau::rtp {

//TODO: Huge key-frames could exceed capacity, so we have to process frames by chucks correctly
using Frame = etl::vector<Buffer, 128>;

}
