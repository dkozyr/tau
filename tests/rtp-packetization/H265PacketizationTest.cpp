#include "tests/rtp-packetization/H265PacketizationBase.h"

namespace tau::rtp {

using namespace h265;

class H265PacketizationTest : public H265PacketizationBase, public ::testing::Test {
};

TEST_F(H265PacketizationTest, Randomized) {
    for(size_t iteration = 0; iteration < 50; ++iteration) {
        _header_options.extension_length_in_words = g_random.Int(0, 8);
        const auto allocator_chunk_size = g_random.Int(128, 1500);
        Init(allocator_chunk_size);
        for(size_t i = 0; i < 10; ++i) {
            auto layer_id = g_random.Int<uint8_t>();
            auto tid = g_random.Int<uint8_t>();
            auto nalu = CreateH265Nalu(NaluType::kPrefixSei, g_random.Int(3, 200'000), layer_id, tid);
            const auto last = g_random.Int(0, 1);
            ASSERT_TRUE(_ctx->packetizer.Process(nalu, last));
            ASSERT_FALSE(_rtp_packets.empty());
            ASSERT_TRUE(_ctx->depacketizer.Process(std::move(_rtp_packets)));
            _rtp_packets.clear();

            ASSERT_EQ(i + 1, _nal_units.size());
            ASSERT_NO_FATAL_FAILURE(AssertBufferView(nalu.GetView(), _nal_units[i].GetView()));
            ASSERT_EQ(kFlagsLast, _nal_units[i].GetInfo().flags);
        }
    }
}

}
