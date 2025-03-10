#include "tests/rtp-packetization/H264PacketizationBase.h"

namespace rtp {

using namespace h264;

class H264PacketizationTest : public H264PacketizationBase, public ::testing::Test {
};

TEST_F(H264PacketizationTest, Randomized) {
    for(size_t iteration = 0; iteration < 50; ++iteration) {
        _header_options.extension_length_in_words = g_random.Int(0, 8);
        const auto allocator_chunk_size = g_random.Int(128, 1500);
        Init(allocator_chunk_size);
        for(size_t i = 0; i < 10; ++i) {
            auto nalu = CreateNalu(NaluType::kNonIdr, g_random.Int(2, 200'000));
            const auto last = g_random.Int(0, 1);
            ASSERT_TRUE(_ctx->packetizer.Process(nalu, last));
            ASSERT_FALSE(_rtp_packets.empty());
            ASSERT_TRUE(_ctx->depacketizer.Process(std::move(_rtp_packets)));
            _rtp_packets.clear();

            ASSERT_EQ(i + 1, _nal_units.size());
            ASSERT_NO_FATAL_FAILURE(AssertBufferView(nalu.GetView(), _nal_units[i].GetView()));
        }
    }
}

}
