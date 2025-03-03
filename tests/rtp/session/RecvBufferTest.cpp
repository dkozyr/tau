#include "tau/rtp/session/RecvBuffer.h"
#include "tau/rtp/RtpAllocator.h"
#include "tau/rtp/Reader.h"
#include "tau/rtp/Writer.h"
#include "tau/memory/SystemAllocator.h"
#include "tau/common/Container.h"
#include "tau/common/Random.h"
#include "tau/common/Log.h"
#include <gtest/gtest.h>
#include <vector>
#include <algorithm>

namespace rtp::session {

class RecvBufferTest : public ::testing::Test {
protected:
    using PacketType = RecvBuffer::PacketType;

protected:
    RecvBufferTest() {
        _recv_buffer.SetCallback([this](Buffer&& packet) {
            _processed_packets.push_back(std::move(packet));
        });
    }

    void PushPackets(const std::vector<uint16_t>& sns, PacketType target_result = PacketType::kOk) {
        for(auto sn : sns) {
            auto packet = CreatePacket(sn);
            ASSERT_EQ(target_result, _recv_buffer.Push(std::move(packet), sn));
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

    void AssertStats(size_t packets, size_t discarded = 0, size_t lost = 0) {
        const auto& stats = _recv_buffer.GetStats();
        ASSERT_EQ(packets, stats.packets);
        ASSERT_EQ(packets * 1200, stats.bytes);
        ASSERT_EQ(discarded, stats.discarded);
        ASSERT_EQ(lost, stats.lost);
    }

    void AssertSnToRecover(const std::vector<uint16_t> sns) {
        const auto& sns_to_recover = _recv_buffer.GetSnsToRecover();
        ASSERT_EQ(sns.size(), sns_to_recover.size());
        for(auto sn : sns) {
            ASSERT_TRUE(Contains(sns_to_recover, sn));
        }
    }

protected:
    RecvBuffer _recv_buffer;
    std::vector<Buffer> _processed_packets;
};

TEST_F(RecvBufferTest, Basic) {
    PushPackets({1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
    ASSERT_NO_FATAL_FAILURE(AssertPacket({1, 2, 3, 4, 5, 6, 7, 8, 9, 10}));
    ASSERT_NO_FATAL_FAILURE(AssertSnToRecover({}));
    ASSERT_NO_FATAL_FAILURE(AssertStats(10));
}

TEST_F(RecvBufferTest, LostPackets) {
    PushPackets({1, 3, 4, 5, 6, 8});
    ASSERT_NO_FATAL_FAILURE(AssertSnToRecover({2, 7}));
    _recv_buffer.Flush();
    ASSERT_NO_FATAL_FAILURE(AssertPacket({1, 3, 4, 5, 6, 8}));
    ASSERT_NO_FATAL_FAILURE(AssertSnToRecover({}));
    ASSERT_NO_FATAL_FAILURE(AssertStats(6, 0, 2));
}

TEST_F(RecvBufferTest, FillBufferOnLostPackets) {
    uint16_t sn = 1;
    std::vector<uint16_t> sns = {sn};
    PushPackets(sns);
    ASSERT_NO_FATAL_FAILURE(AssertStats(1));
    sn = 3;
    while(sn <= RecvBuffer::kDefaultSize + 1) {
        PushPackets({sn});
        sns.push_back(sn);
        ASSERT_NO_FATAL_FAILURE(AssertPacket({1}));
        ASSERT_NO_FATAL_FAILURE(AssertSnToRecover({2}));
        ASSERT_NO_FATAL_FAILURE(AssertStats(sns.size()));
        sn++;
    }

    PushPackets({sn});
    sns.push_back(sn);
    ASSERT_NO_FATAL_FAILURE(AssertPacket(sns));
    ASSERT_NO_FATAL_FAILURE(AssertSnToRecover({}));
    ASSERT_NO_FATAL_FAILURE(AssertStats(sns.size(), 0, 1));
}

TEST_F(RecvBufferTest, FillBufferOnLostPacketAndRecovery) {
    uint16_t sn = 1;
    std::vector<uint16_t> sns = {sn};
    ASSERT_NO_FATAL_FAILURE(PushPackets(sns));
    ASSERT_NO_FATAL_FAILURE(AssertStats(1));
    sn = 3;
    while(sn <= RecvBuffer::kDefaultSize + 1) {
        ASSERT_NO_FATAL_FAILURE(PushPackets({sn}));
        sns.push_back(sn);
        ASSERT_NO_FATAL_FAILURE(AssertPacket({1}));
        ASSERT_NO_FATAL_FAILURE(AssertSnToRecover({2}));
        ASSERT_NO_FATAL_FAILURE(AssertStats(sns.size()));
        sn++;
    }

    ASSERT_NO_FATAL_FAILURE(PushPackets({2}));
    sns.insert(sns.begin() + 1, 2);
    ASSERT_NO_FATAL_FAILURE(AssertPacket(sns));
    ASSERT_NO_FATAL_FAILURE(AssertSnToRecover({}));
    ASSERT_NO_FATAL_FAILURE(AssertStats(sns.size()));
}

TEST_F(RecvBufferTest, LostPacketsWithOverflow) {
    ASSERT_NO_FATAL_FAILURE(PushPackets({65534, 1, 3, 4, 5, 6, 11, 12}));
    ASSERT_NO_FATAL_FAILURE(AssertSnToRecover({65535, 0, 2, 7, 8, 9, 10}));
    _recv_buffer.Flush();
    ASSERT_NO_FATAL_FAILURE(AssertPacket({65534, 1, 3, 4, 5, 6, 11, 12}));
    ASSERT_NO_FATAL_FAILURE(AssertSnToRecover({}));
    ASSERT_NO_FATAL_FAILURE(AssertStats(8, 0, 7));
}

TEST_F(RecvBufferTest, Reordered) {
    ASSERT_NO_FATAL_FAILURE(PushPackets({1, 3, 5, 7, 9, 10, 8, 6, 4, 2}));
    ASSERT_NO_FATAL_FAILURE(AssertPacket({1, 2, 3, 4, 5, 6, 7, 8, 9, 10}));
    ASSERT_NO_FATAL_FAILURE(AssertSnToRecover({}));
    ASSERT_NO_FATAL_FAILURE(AssertStats(10));
}

TEST_F(RecvBufferTest, ReorderedWithOverflow) {
    ASSERT_NO_FATAL_FAILURE(PushPackets({65534, 3, 2, 1, 0, 65535, 6, 5, 7, 4}));
    ASSERT_NO_FATAL_FAILURE(AssertPacket({65534, 65535, 0, 1, 2, 3, 4, 5, 6, 7}));
    ASSERT_NO_FATAL_FAILURE(AssertSnToRecover({}));
    ASSERT_NO_FATAL_FAILURE(AssertStats(10));
}

TEST_F(RecvBufferTest, ReorderedWithDuplicatesAndLost) {
    ASSERT_NO_FATAL_FAILURE(PushPackets({40, 100, 50, 80}));
    ASSERT_NO_FATAL_FAILURE(PushPackets({80}, PacketType::kDiscarded));
    ASSERT_NO_FATAL_FAILURE(PushPackets({110}));
    ASSERT_NO_FATAL_FAILURE(PushPackets({40, 110, 50, 40, 100}, PacketType::kDiscarded));
    _recv_buffer.Flush();
    ASSERT_NO_FATAL_FAILURE(AssertPacket({40, 50, 80, 100, 110}));
    ASSERT_NO_FATAL_FAILURE(AssertSnToRecover({}));
    ASSERT_NO_FATAL_FAILURE(AssertStats(11, 6, 66));
}

TEST_F(RecvBufferTest, Late) {
    ASSERT_NO_FATAL_FAILURE(PushPackets({397, 400, 401, 402, 399, 398}));
    ASSERT_NO_FATAL_FAILURE(PushPackets({100, 101, 102}, PacketType::kDiscarded));
    ASSERT_NO_FATAL_FAILURE(PushPackets({403}));
    ASSERT_NO_FATAL_FAILURE(AssertPacket({397, 398, 399, 400, 401, 402, 403}));
    ASSERT_NO_FATAL_FAILURE(AssertSnToRecover({}));
    ASSERT_NO_FATAL_FAILURE(AssertStats(10, 3));
}

TEST_F(RecvBufferTest, ResetOnBigJump) {
    ASSERT_NO_FATAL_FAILURE(PushPackets({1, 2, 3}));
    ASSERT_NO_FATAL_FAILURE(PushPackets({1000}, PacketType::kReset));
    ASSERT_NO_FATAL_FAILURE(PushPackets({1001, 1002, 1003}));
    ASSERT_NO_FATAL_FAILURE(AssertPacket({1, 2, 3, 1000, 1001, 1002, 1003}));
    ASSERT_NO_FATAL_FAILURE(AssertSnToRecover({}));
    ASSERT_NO_FATAL_FAILURE(AssertStats(7));
}

TEST_F(RecvBufferTest, ResetOnBigJumpWithOverflow) {
    ASSERT_NO_FATAL_FAILURE(PushPackets({50001, 50002, 50003}));
    ASSERT_NO_FATAL_FAILURE(PushPackets({0}, PacketType::kReset));
    ASSERT_NO_FATAL_FAILURE(PushPackets({1, 2, 3}));
    ASSERT_NO_FATAL_FAILURE(AssertPacket({50001, 50002, 50003, 0, 1, 2, 3}));
    ASSERT_NO_FATAL_FAILURE(AssertSnToRecover({}));
    ASSERT_NO_FATAL_FAILURE(AssertStats(7));
}

TEST_F(RecvBufferTest, RandomizedWithoutLoss) {
    Random random;
    size_t total_packets = 0;
    uint16_t sn = random.Int<uint16_t>();
    for(size_t iteration = 0; iteration < 1'000; ++iteration) {
        std::vector<uint16_t> sns;
        const auto packets_count = random.Int<size_t>(1, 50);
        for(size_t i = 0; i < packets_count; ++i) {
            sns.push_back(sn);
            sn++;
        }

        if(iteration != 0) {
            std::vector<uint16_t> sns_shuffled(sns);
            std::random_shuffle(sns_shuffled.begin(), sns_shuffled.end());
            ASSERT_NO_FATAL_FAILURE(PushPackets(sns_shuffled));
        } else {
            ASSERT_NO_FATAL_FAILURE(PushPackets(sns));
        }
        ASSERT_NO_FATAL_FAILURE(AssertPacket(sns));
        ASSERT_NO_FATAL_FAILURE(AssertSnToRecover({}));

        total_packets += sns.size();
        ASSERT_NO_FATAL_FAILURE(AssertStats(total_packets));

        _processed_packets.clear();
    }
}

}
