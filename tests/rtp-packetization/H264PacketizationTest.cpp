#include "H264PacketizationBase.h"
#include <tau/video/AnnexB.h>

namespace tau::rtp {

using namespace h264;

class H264PacketizationTest : public H264PacketizationBase, public ::testing::Test {
};

TEST_F(H264PacketizationTest, Randomized) {
    for(size_t iteration = 0; iteration < 50; ++iteration) {
        _header_options.extension_length_in_words = g_random.Int(0, 8);
        const auto allocator_chunk_size = g_random.Int(128, 1500);
        Init(allocator_chunk_size);
        for(size_t i = 0; i < 10; ++i) {
            const auto nalu_size = allocator_chunk_size * g_random.Int(2, 30);
            auto nalu = CreateH264Nalu(NaluType::kNonIdr, nalu_size);
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

TEST_F(H264PacketizationTest, Randomized_Au) {
    for(size_t iteration = 0; iteration < 50; ++iteration) {
        _header_options.extension_length_in_words = g_random.Int(0, 8);
        const auto allocator_chunk_size = g_random.Int(1200, 1500);
        Init(allocator_chunk_size);
        etl::vector<Buffer, 4> nal_units;
        size_t au_size = 0;
        for(size_t i = 0; i < 4; ++i) {
            const auto nalu_size = allocator_chunk_size * g_random.Int(2, 30);
            auto nalu = CreateH264Nalu(NaluType::kNonIdr, nalu_size);
            nal_units.push_back(std::move(nalu));
            au_size += kAnnexB.size() + nalu_size;
        }

        auto au = Buffer::Create(g_system_allocator, au_size);
        au.SetSize(au_size);
        auto au_view = au.GetView();
        size_t offset = 0;
        for(auto& nalu : nal_units) {
            std::memcpy(&au_view.ptr[offset], kAnnexB.data(), kAnnexB.size());
            offset += kAnnexB.size();
            std::memcpy(&au_view.ptr[offset], nalu.GetView().ptr, nalu.GetView().size);
            offset += nalu.GetView().size;
        }

        ASSERT_TRUE(_ctx->packetizer.Process(au));
        ASSERT_FALSE(_rtp_packets.empty());
        ASSERT_TRUE(_ctx->depacketizer.Process(std::move(_rtp_packets)));

        ASSERT_EQ(nal_units.size(), _nal_units.size());
        for(size_t i = 0; i < nal_units.size(); ++i) {
            ASSERT_NO_FATAL_FAILURE(AssertBufferView(nal_units[i].GetView(), _nal_units[i].GetView()));
            if(i + 1 == nal_units.size()) {
                ASSERT_EQ(kFlagsLast, _nal_units[i].GetInfo().flags);
            } else {
                ASSERT_EQ(kFlagsNone, _nal_units[i].GetInfo().flags);
            }
        }
    }
}

}
