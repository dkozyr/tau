#include "tau/memory/Buffer.h"

namespace tau::rtp {

Buffer CreatePacket(uint32_t ts, uint16_t sn, bool marker);

}
