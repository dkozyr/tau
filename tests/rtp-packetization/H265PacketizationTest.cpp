#include "H265PacketizationBase.h"
#include <tau/video/AnnexB.h>

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
            const auto nalu_size = allocator_chunk_size * g_random.Int(2, 30);
            auto nalu = CreateH265Nalu(NaluType::kPrefixSei, nalu_size, layer_id, tid);
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

TEST_F(H265PacketizationTest, Randomized_Au) {
    for(size_t iteration = 0; iteration < 50; ++iteration) {
        _header_options.extension_length_in_words = g_random.Int(0, 8);
        const auto allocator_chunk_size = g_random.Int(1200, 1500);
        Init(allocator_chunk_size);
        etl::vector<Buffer, 9> nal_units;
        size_t au_size = 0;
        for(size_t index = 0; index < 9; ++index) {
            const auto nal_unit_size = (index % 2 == 0) ? g_random.Int(3, 64) : allocator_chunk_size * g_random.Int(2, 10);
            auto nal_unit = CreateH265Nalu(NaluType::kPrefixSei, nal_unit_size, g_random.Int<uint8_t>(), g_random.Int<uint8_t>());
            nal_units.push_back(std::move(nal_unit));
            au_size += kAnnexB.size() + nal_unit_size;
        }

        auto au = Buffer::Create(g_system_allocator, au_size);
        au.SetSize(au_size);
        auto au_view = au.GetView();
        size_t offset = 0;
        for(auto& nal_unit : nal_units) {
            std::memcpy(&au_view.ptr[offset], kAnnexB.data(), kAnnexB.size());
            offset += kAnnexB.size();
            std::memcpy(&au_view.ptr[offset], nal_unit.GetView().ptr, nal_unit.GetView().size);
            offset += nal_unit.GetView().size;
        }

        ASSERT_TRUE(_ctx->packetizer.Process(au));
        ASSERT_FALSE(_rtp_packets.empty());
        for(size_t packet_index = 0; packet_index < _rtp_packets.size(); ++packet_index) {
            const auto& packet = _rtp_packets[packet_index];
            auto packet_view = packet.GetView();
            ASSERT_TRUE(Reader::Validate(packet_view));
            Reader reader(packet_view);
            ASSERT_EQ(packet_index + 1 == _rtp_packets.size(), reader.Marker()) << "Packet index: " << packet_index;
        }
        ASSERT_TRUE(_ctx->depacketizer.Process(std::move(_rtp_packets)));

        ASSERT_EQ(nal_units.size(), _nal_units.size());
        for(size_t index = 0; index < nal_units.size(); ++index) {
            ASSERT_NO_FATAL_FAILURE(AssertBufferView(nal_units[index].GetView(), _nal_units[index].GetView()));
            if(index + 1 == nal_units.size()) {
                ASSERT_EQ(kFlagsLast, _nal_units[index].GetInfo().flags);
            } else {
                ASSERT_EQ(kFlagsNone, _nal_units[index].GetInfo().flags);
            }
        }
    }
}

}
