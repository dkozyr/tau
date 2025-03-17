#include "tests/lib/RtpUtils.h"
#include "tests/lib/Common.h"
#include "tau/rtp/Writer.h"

namespace tau::rtp {

Buffer CreatePacket(uint32_t ts, uint16_t sn, bool marker) {
    auto packet = Buffer::Create(g_udp_allocator, 1200);
    Writer::Write(packet.GetViewWithCapacity(), Writer::Options{
        .pt = 96,
        .ssrc = 0x11223344,
        .ts = ts,
        .sn = sn,
        .marker = marker
    });
    packet.SetSize(1200);
    return packet;
}

}
