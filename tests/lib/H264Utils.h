#include "tau/video/h264/Nalu.h"
#include "tau/memory/Buffer.h"

namespace tau::h264 {

Buffer CreateNalu(NaluType type, size_t size = 256);

}
