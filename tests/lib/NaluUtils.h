#include "tau/video/h264/Nalu.h"
#include "tau/video/h265/Nalu.h"
#include "tau/memory/Buffer.h"

namespace tau {

Buffer CreateH264Nalu(h264::NaluType type, size_t size = 256);
Buffer CreateH265Nalu(h265::NaluType type, size_t size = 256, uint8_t layer_id = 0, uint8_t tid = 0);

}
