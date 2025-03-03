#include "tau/rtp/session/SendBuffer.h"
#include "tau/rtp/RtpAllocator.h"
#include "tau/rtp/Reader.h"
#include "tau/rtp/Writer.h"
#include "tau/memory/SystemAllocator.h"
#include "tau/common/Random.h"
#include "tau/common/Log.h"
#include <gtest/gtest.h>
#include <vector>
#include <algorithm>

namespace rtp::session {

class SendBufferTest : public ::testing::Test {
protected:
    static constexpr size_t kSmallCapacity = 7;

protected:
    SendBufferTest()
        : _send_buffer(kSmallCapacity) {
        _send_buffer.SetCallback([this](Buffer&& packet) {
            // Reader reader(ToConst(packet.GetView()));
            // LOG_INFO << "processed sn: " << reader.Sn();
            _processed_packets.push_back(std::move(packet));
        });
    }

    void PushPackets(size_t count, uint16_t sn_begin = 1) {
        uint16_t sn = sn_begin;
        for(size_t i = 0; i < count; ++i, ++sn) {
            auto packet = CreatePacket(sn);
            _send_buffer.Push(std::move(packet), sn);
        }
    }

    Buffer CreatePacket(uint16_t sn, uint32_t ts = 0) {
        auto packet = Buffer::Create(g_system_allocator, 1200);
        Writer::Write(packet.GetViewWithCapacity(), Writer::Options{
            .pt = 96,
            .ssrc = 0x11223344,
            .ts = ts,
            .sn = sn,
            .marker = false
        });
        packet.SetSize(1200);
        return packet;
    }

    void AssertPacket(const std::vector<uint16_t>& sns) {
        ASSERT_EQ(sns.size(), _processed_packets.size());
        for(size_t i = 0; i < sns.size(); ++i) {
            const auto& packet = _processed_packets[i];
            Reader reader(packet.GetView());
            ASSERT_EQ(sns[i], reader.Sn());
        }
    }

    void AssertSendRtxSuccessful(const std::vector<uint16_t>& sns) {
        for(auto sn : sns) {
            const auto processed_packets = _processed_packets.size();
            ASSERT_TRUE(_send_buffer.SendRtx(sn));
            ASSERT_EQ(processed_packets + 1, _processed_packets.size());

            const auto& packet = _processed_packets.back();
            Reader reader(packet.GetView());
            ASSERT_EQ(sn, reader.Sn());
        }
    }

    void AssertSendRtxFailed(const std::vector<uint16_t>& sns) {
        for(auto sn : sns) {
            const auto processed_packets = _processed_packets.size();
            ASSERT_FALSE(_send_buffer.SendRtx(sn));
            ASSERT_EQ(processed_packets, _processed_packets.size());
        }
    }

    void AssertStats(size_t target_packets, size_t target_rtx) {
        const auto& stats = _send_buffer.GetStats();
        ASSERT_EQ(target_packets, stats.packets);
        ASSERT_EQ(target_rtx, stats.rtx);
        ASSERT_EQ(target_packets * 1200, stats.bytes);
    }

protected:
    SendBuffer _send_buffer;
    std::vector<Buffer> _processed_packets;
};

TEST_F(SendBufferTest, Basic) {
    PushPackets(10);
    ASSERT_NO_FATAL_FAILURE(AssertPacket({1, 2, 3, 4, 5, 6, 7, 8, 9, 10}));
    ASSERT_NO_FATAL_FAILURE(AssertStats(10, 0));
    ASSERT_NO_FATAL_FAILURE(AssertSendRtxSuccessful({4, 5, 6, 7, 8, 9, 10}));
    ASSERT_NO_FATAL_FAILURE(AssertStats(17, 7));
    ASSERT_NO_FATAL_FAILURE(AssertSendRtxSuccessful({10, 4, 9, 5, 8, 6, 7}));
    ASSERT_NO_FATAL_FAILURE(AssertStats(24, 14));
    ASSERT_NO_FATAL_FAILURE(AssertSendRtxFailed({0, 1, 2, 3, 11}));
    ASSERT_NO_FATAL_FAILURE(AssertStats(24, 14));
}

}
