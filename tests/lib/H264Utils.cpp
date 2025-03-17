#include "tests/lib/H264Utils.h"
#include "tests/lib/Common.h"

namespace tau::h264 {

Buffer CreateNalu(NaluType type, size_t size) {
    auto nalu = Buffer::Create(g_system_allocator, size);
    auto view = nalu.GetViewWithCapacity();
    view.ptr[0] = CreateNalUnitHeader(type, 0b11);
    for(size_t i = 1; i < size; ++i) {
        view.ptr[i] = i;
    }
    nalu.SetSize(size);
    return nalu;
}

}
