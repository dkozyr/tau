#include "tests/lib/NaluUtils.h"
#include "tests/lib/Common.h"

namespace tau {

Buffer CreateH264Nalu(h264::NaluType type, size_t size) {
    auto nalu = Buffer::Create(g_system_allocator, size);
    auto view = nalu.GetViewWithCapacity();
    view.ptr[0] = h264::CreateNalUnitHeader(type, 0b11);
    for(size_t i = 1; i < size; ++i) {
        view.ptr[i] = i;
    }
    nalu.SetSize(size);
    return nalu;
}

Buffer CreateH265Nalu(h265::NaluType type, size_t size, uint8_t layer_id, uint8_t tid) {
    if(size < h265::kNaluHeaderSize) {
        TAU_EXCEPTION(std::runtime_error, "Wrong size: " << size);
    }
    auto nalu = Buffer::Create(g_system_allocator, size);
    auto view = nalu.GetViewWithCapacity();
    h265::SetNaluHeaderUnsafe(view.ptr, type, layer_id, tid);
    for(size_t i = h265::kNaluHeaderSize; i < size; ++i) {
        view.ptr[i] = i;
    }
    nalu.SetSize(size);
    return nalu;
}

}
