#include <tau/rtp-packetization/Av1Packetizer.h>
#include <tau/rtp-packetization/Av1AggregationHeader.h>
#include <array>
#include <limits>
#include <tau/rtp-packetization/Av1Depacketizer.h>
#include <tau/rtp/Reader.h>
#include <tau/video/av1/Obu.h>
#include <tau/video/av1/Leb128.h>
#include <tests/lib/Common.h>

namespace tau::rtp {

class Av1PacketizationTest : public ::testing::Test {
protected:
    static constexpr uint8_t kFrameHeader              = av1::ObuType::kFrame << 3;
    static constexpr uint8_t kSizedFrameHeader         = kFrameHeader | av1::kObuHasSizeMask;
    static constexpr uint8_t kExtendedFrameHeader      = kFrameHeader | av1::kObuExtensionMask;
    static constexpr uint8_t kSizedExtendedFrameHeader = kExtendedFrameHeader | av1::kObuHasSizeMask;
    static constexpr uint8_t kSequenceHeader           = (av1::ObuType::kSequenceHeader << 3) | av1::kObuHasSizeMask;
    static constexpr uint8_t kTemporalDelimiterHeader  = av1::ObuType::kTemporalDelimiter << 3;
    static constexpr uint8_t kSizedTemporalDelimiterHeader = kTemporalDelimiterHeader | av1::kObuHasSizeMask;
    static constexpr uint8_t kTileListHeader           = av1::ObuType::kTileList << 3;
    static constexpr uint8_t kSizedTileListHeader      = kTileListHeader | av1::kObuHasSizeMask;
    static constexpr uint8_t kExtensionHeader          = 0x28;
    static constexpr uint8_t kInvalidFrameHeader       = kFrameHeader | av1::kObuForbiddenMask;
    static constexpr uint8_t kAllElementsSized         = 0;
    static constexpr uint8_t kOneElement               = 0b00010000;
    static constexpr uint8_t kTwoElements              = 0b00100000;
    static constexpr uint8_t kThreeElements            = 0b00110000;
    static constexpr uint8_t kFirstFragment            = kOneElement | kAv1ContinuesMask;
    static constexpr uint8_t kLastFragment             = kOneElement | kAv1ContinuationMask;
    static constexpr uint8_t kMiddleAggregation        = kTwoElements | kAv1ContinuationMask | kAv1ContinuesMask;
    static constexpr uint8_t kLastAggregation          = kTwoElements | kAv1ContinuationMask;
    static constexpr uint8_t kInvalidSequenceContinuation = kLastFragment | kAv1NewSequenceMask;
    static constexpr uint8_t kLeb128FullContinuation   = av1::kLeb128ValueMask | av1::kLeb128ContinuationMask;
    static constexpr uint8_t kLeb128OneContinuation    = 1 | av1::kLeb128ContinuationMask;

protected:
    Av1PacketizationTest()
        : _allocator(g_udp_allocator, RtpAllocator::Options{
            .header = {.pt = 96, .ssrc = 123, .ts = 456, .sn = std::numeric_limits<uint16_t>::max(), .marker = false},
            .base_tp = 0,
            .clock_rate = 90'000
        })
        , _packetizer(_allocator)
        , _depacketizer(g_system_allocator) {
        _packetizer.SetCallback([this](Buffer&& packet) { _packets.push_back(std::move(packet)); });
        _depacketizer.SetCallback([this](Buffer&& unit) { _units.push_back(std::move(unit)); });
    }

    Buffer CreateUnit(size_t payload_size, uint8_t header = kSizedFrameHeader) {
        std::vector<uint8_t> bytes(payload_size + 10);
        bytes[0] = header;
        size_t offset = 1;
        if(header & av1::kObuExtensionMask) {
            bytes[offset++] = kExtensionHeader;
        }
        if(header & av1::kObuHasSizeMask) {
            offset += av1::WriteLeb128(bytes.data() + offset, payload_size);
        }
        for(size_t index = 0; index < payload_size; ++index) {
            bytes[offset + index] = static_cast<uint8_t>(index);
        }
        return Buffer::Create(g_system_allocator, {bytes.data(), offset + payload_size}, Buffer::Info{.tp = 1234});
    }

    void AddPacket(std::initializer_list<uint8_t> bytes, bool marker = false, Timepoint timepoint = 1234) {
        auto packet = _allocator.Allocate(timepoint, marker);
        const auto header_size = packet.GetSize();
        memcpy(packet.GetView().ptr + header_size, bytes.begin(), bytes.size());
        packet.SetSize(header_size + bytes.size());
        _packets.push_back(std::move(packet));
    }

    void AssertBytes(const Buffer& buffer, std::initializer_list<uint8_t> bytes) {
        ASSERT_EQ(bytes.size(), buffer.GetSize());
        ASSERT_EQ(0, memcmp(bytes.begin(), buffer.GetView().ptr, bytes.size()));
    }

protected:
    RtpAllocator _allocator;
    Av1Packetizer _packetizer;
    Av1Depacketizer _depacketizer;
    Frame _packets;
    std::vector<Buffer> _units;
};

TEST_F(Av1PacketizationTest, SingleWireFormat) {
    auto unit = CreateUnit(3);
    ASSERT_TRUE(_packetizer.Process(unit, true));
    ASSERT_EQ(1, _packets.size());
    Reader reader(std::as_const(_packets[0]).GetView());

    const auto payload = reader.Payload();
    const std::array<uint8_t, 5> expected = {kOneElement, kFrameHeader, 0, 1, 2};
    ASSERT_EQ(expected.size(), payload.size);
    ASSERT_EQ(0, memcmp(expected.data(), payload.ptr, payload.size));
    ASSERT_TRUE(reader.Marker());

    ASSERT_TRUE(_depacketizer.Process(std::move(_packets)));
    ASSERT_EQ(1, _units.size());
    AssertBytes(_units[0], {kSizedFrameHeader, 3, 0, 1, 2});
    ASSERT_EQ(1234, _units[0].GetInfo().tp);
    ASSERT_EQ(kFlagsLast, _units[0].GetInfo().flags);
}

TEST_F(Av1PacketizationTest, FragmentRoundTripAndSequenceWrap) {
    const std::array<size_t, 7> payload_sizes = {0, 1, 127, 128, 1400, 8000, 70000};
    for(const auto payload_size : payload_sizes) {
        SCOPED_TRACE(payload_size);
        auto unit = CreateUnit(payload_size, kSizedExtendedFrameHeader);
        ASSERT_TRUE(_packetizer.Process(unit, true));

        for(size_t index = 0; index < _packets.size(); ++index) {
            Reader reader(std::as_const(_packets[index]).GetView());
            const auto payload = reader.Payload();
            ASSERT_LE(payload.size, _allocator.MaxRtpPayload());
            ASSERT_EQ(index != 0, bool(payload.ptr[0] & kAv1ContinuationMask));
            ASSERT_EQ(index + 1 != _packets.size(), bool(payload.ptr[0] & kAv1ContinuesMask));
            ASSERT_EQ(index + 1 == _packets.size(), reader.Marker());
        }

        ASSERT_TRUE(_depacketizer.Process(std::move(_packets)));
        ASSERT_EQ(1, _units.size());
        ASSERT_EQ(unit.GetSize(), _units[0].GetSize());
        ASSERT_EQ(0, memcmp(unit.GetView().ptr, _units[0].GetView().ptr, unit.GetSize()));

        _packets.clear();
        _units.clear();
    }
}

TEST_F(Av1PacketizationTest, TemporalUnitAndSequenceStart) {
    const std::array<uint8_t, 12> bytes = {
        kSizedTemporalDelimiterHeader, 0, kSequenceHeader, 1, 7,
        kSizedFrameHeader, 1, 8,
        kSizedTileListHeader, 0,
        kSizedTemporalDelimiterHeader, 0
    };
    auto unit = Buffer::Create(g_system_allocator, {bytes.data(), bytes.size()});
    _packetizer.StartSequence();
    ASSERT_TRUE(_packetizer.Process(unit));
    ASSERT_EQ(1, _packets.size());
    ASSERT_EQ(kAv1NewSequenceMask, Reader(std::as_const(_packets[0]).GetView()).Payload().ptr[0]);
    ASSERT_TRUE(Reader(std::as_const(_packets[0]).GetView()).Marker());

    ASSERT_TRUE(_depacketizer.Process(std::move(_packets)));
    ASSERT_EQ(2, _units.size());
    AssertBytes(_units[0], {kSequenceHeader, 1, 7});
    AssertBytes(_units[1], {kSizedFrameHeader, 1, 8});
}

TEST_F(Av1PacketizationTest, AggregationAllCountModes) {
    for(const uint8_t count : {0, 1, 2, 3}) {
        _packets.clear();
        _units.clear();
        if(count == 0) {
            AddPacket({kAllElementsSized, 2, kFrameHeader, 7, 2, kFrameHeader, 8, 2, kFrameHeader, 9, 2, kFrameHeader, 10}, true);
        } else if(count == 1) {
            AddPacket({kOneElement, kFrameHeader, 7}, true);
        } else if(count == 2) {
            AddPacket({kTwoElements, 2, kFrameHeader, 7, kFrameHeader, 8}, true);
        } else {
            AddPacket({kThreeElements, 2, kFrameHeader, 7, 2, kFrameHeader, 8, kFrameHeader, 9}, true);
        }
        ASSERT_TRUE(_depacketizer.Process(std::move(_packets)));
        ASSERT_EQ(count ? count : 4, _units.size());
        for(size_t index = 0; index < _units.size(); ++index) {
            AssertBytes(_units[index], {kSizedFrameHeader, 1, static_cast<uint8_t>(7 + index)});
            ASSERT_EQ(index + 1 == _units.size() ? kFlagsLast : kFlagsNone, _units[index].GetInfo().flags);
        }
    }
}

TEST_F(Av1PacketizationTest, MixedAggregationAndFragmentsAcrossCalls) {
    AddPacket({kFirstFragment, kFrameHeader, 1});
    ASSERT_TRUE(_depacketizer.Process(std::move(_packets)));
    ASSERT_TRUE(_units.empty());
    _packets.clear();

    AddPacket({kMiddleAggregation, 1, 2, kFrameHeader, 3});
    ASSERT_TRUE(_depacketizer.Process(std::move(_packets)));
    ASSERT_EQ(1, _units.size());
    AssertBytes(_units[0], {kSizedFrameHeader, 2, 1, 2});
    ASSERT_EQ(kFlagsNone, _units[0].GetInfo().flags);
    _packets.clear();

    AddPacket({kLastAggregation, 1, 4, kFrameHeader, 5}, true);
    ASSERT_TRUE(_depacketizer.Process(std::move(_packets)));
    ASSERT_EQ(3, _units.size());
    AssertBytes(_units[1], {kSizedFrameHeader, 2, 3, 4});
    AssertBytes(_units[2], {kSizedFrameHeader, 1, 5});
    ASSERT_EQ(kFlagsLast, _units[2].GetInfo().flags);
}

TEST_F(Av1PacketizationTest, MissingFragmentAndRecovery) {
    auto unit = CreateUnit(8000);
    ASSERT_TRUE(_packetizer.Process(unit, true));
    _packets.erase(_packets.begin() + 1);
    ASSERT_FALSE(_depacketizer.Process(std::move(_packets)));
    ASSERT_TRUE(_units.empty());
    _packets.clear();

    AddPacket({kOneElement, kFrameHeader, 9}, true);
    ASSERT_TRUE(_depacketizer.Process(std::move(_packets)));
    ASSERT_EQ(1, _units.size());
}

TEST_F(Av1PacketizationTest, InvalidPayloads) {
    const std::vector<std::vector<uint8_t>> cases = {
        {},
        {kOneElement},
        {kOneElement, kInvalidFrameHeader},
        {kOneElement, kExtendedFrameHeader},
        {kOneElement, kExtendedFrameHeader, 1},
        {kOneElement, kSizedFrameHeader, 4, 1},
        {kAllElementsSized, 0},
        {kAllElementsSized, 3, kFrameHeader, 1},
        {kAllElementsSized, av1::kLeb128ContinuationMask},
        {kTwoElements, 2, kFrameHeader, 1},
        {kAllElementsSized, kLeb128FullContinuation, kLeb128FullContinuation,
            kLeb128FullContinuation, kLeb128FullContinuation, av1::kLeb128ValueMask},
        {kLastFragment, 1},
        {kInvalidSequenceContinuation, 1},
        {kFirstFragment, kFrameHeader, 1}
    };

    for(const auto& bytes : cases) {
        _packets.clear();
        auto packet = _allocator.Allocate(1234, true);
        const auto header_size = packet.GetSize();
        if(!bytes.empty()) {
            memcpy(packet.GetView().ptr + header_size, bytes.data(), bytes.size());
        }
        packet.SetSize(header_size + bytes.size());
        _packets.push_back(std::move(packet));
        ASSERT_FALSE(_depacketizer.Process(std::move(_packets)));
        ASSERT_TRUE(_units.empty());
    }
}

TEST_F(Av1PacketizationTest, SizeLimit) {
    constexpr size_t kMaxUnitSize = 3;
    Av1Depacketizer limited(g_system_allocator, kMaxUnitSize);
    limited.SetCallback([this](Buffer&& unit) { _units.push_back(std::move(unit)); });
    AddPacket({kFirstFragment, kFrameHeader, 1, 2});
    ASSERT_TRUE(limited.Process(std::move(_packets)));
    _packets.clear();

    AddPacket({kLastFragment, 3}, true);
    ASSERT_FALSE(limited.Process(std::move(_packets)));
    ASSERT_TRUE(_units.empty());
}

TEST_F(Av1PacketizationTest, SizedWireObuAndIgnoredTypes) {
    AddPacket({kThreeElements, 3, kSizedFrameHeader, 1, 9, 1, kTemporalDelimiterHeader, kTileListHeader}, true);
    ASSERT_TRUE(_depacketizer.Process(std::move(_packets)));
    ASSERT_EQ(1, _units.size());
    AssertBytes(_units[0], {kSizedFrameHeader, 1, 9});
    ASSERT_EQ(kFlagsLast, _units[0].GetInfo().flags);
}

TEST_F(Av1PacketizationTest, MalformedTemporalUnitProducesNoPackets) {
    const std::array<uint8_t, 6> bytes = {kSizedFrameHeader, 1, 0, kSizedFrameHeader, 3, 0};
    auto unit = Buffer::Create(g_system_allocator, {bytes.data(), bytes.size()});
    ASSERT_FALSE(_packetizer.Process(unit));
    ASSERT_TRUE(_packets.empty());
}

TEST_F(Av1PacketizationTest, FragmentedSequenceStartOnlyOnFirstPacket) {
    auto unit = CreateUnit(8000, kSequenceHeader);
    _packetizer.StartSequence();
    ASSERT_TRUE(_packetizer.Process(unit, true));
    ASSERT_GT(_packets.size(), 1);
    for(size_t index = 0; index < _packets.size(); ++index) {
        const Reader reader(std::as_const(_packets[index]).GetView());
        ASSERT_EQ(index == 0, bool(reader.Payload().ptr[0] & kAv1NewSequenceMask));
    }
    ASSERT_TRUE(_depacketizer.Process(std::move(_packets)));
    ASSERT_EQ(1, _units.size());
}

TEST_F(Av1PacketizationTest, TimestampChangeDiscardsFragment) {
    AddPacket({kFirstFragment, kFrameHeader, 1});
    ASSERT_TRUE(_depacketizer.Process(std::move(_packets)));
    _packets.clear();
    AddPacket({kLastFragment, 2}, true, kSec);
    ASSERT_FALSE(_depacketizer.Process(std::move(_packets)));
    ASSERT_TRUE(_units.empty());
}

TEST_F(Av1PacketizationTest, ExtensionHeaderSplitAcrossPackets) {
    AddPacket({kFirstFragment, kExtendedFrameHeader});
    AddPacket({kLastFragment, kExtensionHeader, 1, 2}, true);
    ASSERT_TRUE(_depacketizer.Process(std::move(_packets)));
    ASSERT_EQ(1, _units.size());
    AssertBytes(_units[0], {kSizedExtendedFrameHeader, kExtensionHeader, 2, 1, 2});
}

TEST_F(Av1PacketizationTest, MultibyteAggregationLength) {
    constexpr size_t kFirstPayloadSize    = 128;
    constexpr size_t kFirstPayloadOffset  = 4;
    constexpr size_t kSecondHeaderOffset  = kFirstPayloadOffset + kFirstPayloadSize;
    constexpr size_t kSecondPayloadOffset = kSecondHeaderOffset + 1;
    constexpr size_t kPacketPayloadSize   = kSecondPayloadOffset + 1;
    constexpr size_t kRestoredUnitSize    = 1 + 2 + kFirstPayloadSize;
    constexpr uint8_t kFirstPayloadByte   = 7;
    constexpr uint8_t kSecondPayloadByte  = 8;

    auto packet = _allocator.Allocate(1234, true);
    const auto header_size = packet.GetSize();
    auto payload = packet.GetView().ptr + header_size;
    payload[0] = kTwoElements;
    payload[1] = kLeb128OneContinuation;
    payload[2] = 1;
    payload[3] = kFrameHeader;
    memset(payload + kFirstPayloadOffset, kFirstPayloadByte, kFirstPayloadSize);
    payload[kSecondHeaderOffset] = kFrameHeader;
    payload[kSecondPayloadOffset] = kSecondPayloadByte;
    packet.SetSize(header_size + kPacketPayloadSize);
    _packets.push_back(std::move(packet));

    ASSERT_TRUE(_depacketizer.Process(std::move(_packets)));
    ASSERT_EQ(2, _units.size());
    ASSERT_EQ(kRestoredUnitSize, _units[0].GetSize());
    ASSERT_EQ(av1::kLeb128ContinuationMask, _units[0].GetView().ptr[1]);
    ASSERT_EQ(1, _units[0].GetView().ptr[2]);
    AssertBytes(_units[1], {kSizedFrameHeader, 1, kSecondPayloadByte});
}

TEST_F(Av1PacketizationTest, UnsizedInputAndMalformedUnit) {
    auto unit = CreateUnit(3, kFrameHeader);
    ASSERT_FALSE(_packetizer.Process(unit));
    ASSERT_TRUE(_packets.empty());
    ASSERT_TRUE(_packetizer.Process(unit, true));
    ASSERT_TRUE(_depacketizer.Process(std::move(_packets)));
    ASSERT_EQ(1, _units.size());
    AssertBytes(_units[0], {kSizedFrameHeader, 3, 0, 1, 2});
}

TEST_F(Av1PacketizationTest, RecoveryKeyframesKeepSequenceHeaderAndFrameTogether) {
    const std::array<uint8_t, 8> bytes = {
        kSizedTemporalDelimiterHeader, 0, kSequenceHeader, 1, 7, kSizedFrameHeader, 1, 8}
    ;
    auto unit = Buffer::Create(g_system_allocator, {bytes.data(), bytes.size()});
    constexpr size_t kKeyframeCount = 3;
    for(size_t keyframe = 0; keyframe < kKeyframeCount; ++keyframe) {
        ASSERT_TRUE(_packetizer.Process(unit));
        ASSERT_EQ(1, _packets.size());
        const Reader reader(std::as_const(_packets[0]).GetView());
        EXPECT_EQ(kAv1NewSequenceMask, reader.Payload().ptr[0]);
        EXPECT_TRUE(reader.Marker());
        _packets.clear();
    }
}

TEST_F(Av1PacketizationTest, FragmentedAccessUnitHasOnlyOneFrameBoundary) {
    const std::array<size_t, 5> payload_sizes = {1100, 1180, 1190, 2400, 8000};
    for(const auto payload_size : payload_sizes) {
        auto sequence_header = CreateUnit(payload_size, kSequenceHeader);
        auto frame = CreateUnit(2400);
        std::vector<uint8_t> bytes;
        for(const auto* unit : {&sequence_header, &frame}) {
            const auto view = unit->GetView();
            bytes.insert(bytes.end(), view.ptr, view.ptr + view.size);
        }
        _packets.clear();
        _units.clear();

        ASSERT_TRUE(_packetizer.Process(Buffer::Create(g_system_allocator, {bytes.data(), bytes.size()})));
        ASSERT_GT(_packets.size(), 1);
        for(size_t index = 0; index < _packets.size(); ++index) {
            const Reader reader(std::as_const(_packets[index]).GetView());
            EXPECT_EQ(index != 0, bool(reader.Payload().ptr[0] & kAv1ContinuationMask));
            EXPECT_EQ(index + 1 != _packets.size(), bool(reader.Payload().ptr[0] & kAv1ContinuesMask));
            EXPECT_EQ(index == 0, bool(reader.Payload().ptr[0] & kAv1NewSequenceMask));
            EXPECT_EQ(index + 1 == _packets.size(), reader.Marker());
        }

        ASSERT_TRUE(_depacketizer.Process(std::move(_packets)));
        ASSERT_EQ(2, _units.size());
        EXPECT_EQ(sequence_header.GetSize(), _units[0].GetSize());
        EXPECT_EQ(frame.GetSize(), _units[1].GetSize());
        EXPECT_EQ(0, memcmp(sequence_header.GetView().ptr, _units[0].GetView().ptr, sequence_header.GetSize()));
        EXPECT_EQ(0, memcmp(frame.GetView().ptr, _units[1].GetView().ptr, frame.GetSize()));
    }
}

}
