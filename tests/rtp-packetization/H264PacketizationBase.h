#include "tau/rtp-packetization/H264Packetizer.h"
#include "tau/rtp-packetization/H264Depacketizer.h"
#include "tau/rtp/Reader.h"
#include "tau/rtp/Constants.h"
#include "tau/video/h264/Nalu.h"
#include "tests/lib/Common.h"
#include "tests/lib/H264Utils.h"

namespace tau::rtp {

using namespace h264;

class H264PacketizationBase {
protected:
    static constexpr uint32_t kDefaultClockRate = 90'000;

public:
    H264PacketizationBase() {
        static_assert(sizeof(FuAIndicator) == 1);
        static_assert(sizeof(FuAHeader) == 1);

        Init();
    }

protected:
    void Init(size_t allocator_chunk_size = 1200) {
        _rtp_packets.clear();
        _nal_units.clear();

        _ctx.emplace(_header_options, _clock.Now(), allocator_chunk_size);
        _ctx->packetizer.SetCallback([this](Buffer&& rtp_packet) {
            _rtp_packets.push_back(std::move(rtp_packet));
        });
        _ctx->depacketizer.SetCallback([this](Buffer&& nalu) {
            _nal_units.push_back(std::move(nalu));
        });
    }

    static void AssertBufferView(const BufferView& target, const BufferView& actual) {
        ASSERT_EQ(target.size, actual.size);
        ASSERT_EQ(0, std::memcmp(target.ptr, actual.ptr, target.size));
    }

protected:
    Writer::Options _header_options = {
        .pt = 96,
        .ssrc = 0x11223344,
        .ts = 1234567890,
        .sn = 65535,
        .marker = false
    };

    SteadyClock _clock;

    struct Context {
        Context(const Writer::Options& options, Timepoint tp, size_t allocator_chunk_size)
            : allocator(RtpAllocator::Options{
                .header = options,
                .base_tp = tp,
                .clock_rate = kDefaultClockRate,
                .size = allocator_chunk_size
            })
            , packetizer(allocator)
            , depacketizer(g_system_allocator)
        {}

        RtpAllocator allocator;
        H264Packetizer packetizer;
        H264Depacketizer depacketizer;
    };
    std::optional<Context> _ctx;
    Frame _rtp_packets;
    std::vector<Buffer> _nal_units;
};

}
