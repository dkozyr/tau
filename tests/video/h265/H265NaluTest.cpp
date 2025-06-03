#include "tau/video/h265/Nalu.h"
#include "tests/lib/Common.h"

namespace tau::h265 {

TEST(H265NaluTest, SetNaluHeader) {
    std::array<uint8_t, 2> header;
    const auto nalu_type = NaluType::kBlaWLp;
    const auto layer_id = g_random.Int<uint8_t>(0, 63);
    const auto tid = g_random.Int<uint8_t>(0, 7);
    SetNaluHeaderUnsafe(header.data(), nalu_type, layer_id, tid);
    ASSERT_EQ(nalu_type, GetNaluTypeUnsafe(header.data()));
    ASSERT_EQ(layer_id, GetNaluLayerUnsafe(header.data()));
    ASSERT_EQ(tid, GetNaluTidUnsafe(header.data()));
}

}
