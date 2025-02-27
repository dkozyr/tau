#include <tau/rtp/H264Packetizer.h>
#include <tau/video/h264/Nalu.h>
#include <tau/common/Math.h>

namespace rtp {

H264Packetizer::H264Packetizer(RtpAllocator& allocator)
    : _allocator(allocator)
    , _max_payload(_allocator.MaxRtpPayload())
{}

bool H264Packetizer::Process(const Buffer& nal_unit, bool last) {
    auto view = nal_unit.GetView();
    if(view.size <= sizeof(h264::NaluHeader)) {
        return false;
    }
    auto header = reinterpret_cast<const h264::NaluHeader*>(&view.ptr[0]);
    if(header->forbidden || (header->type >= h264::NaluType::kStapA)) {
        return false;
    }

    const auto tp = nal_unit.GetInfo().tp;
    if(view.size <= _max_payload) {
        ProcessSingle(view, tp, last);
    } else {
        ProcessFuA(view, tp, last);
    }
    return true;
}

void H264Packetizer::ProcessSingle(const BufferViewConst& view, Timepoint tp, bool last) {
    auto packet = _allocator.Allocate(tp, last);
    auto header_size = packet.GetSize();
    auto payload_ptr = packet.GetView().ptr + header_size;
    memcpy(payload_ptr, view.ptr, view.size);
    packet.SetSize(header_size + view.size);
    _callback(std::move(packet));
}

void H264Packetizer::ProcessFuA(const BufferViewConst& view, Timepoint tp, bool last) {
    auto nalu_payload = view;
    nalu_payload.ForwardPtrUnsafe(sizeof(h264::NaluHeader));

    const auto max_fua_payload = _max_payload - 2 * sizeof(uint8_t); //rtp payload - FuA indicator - FuA header
    const auto packets_count = DivCeil(nalu_payload.size, max_fua_payload);

    for(size_t i = 1; i <= packets_count; ++i) {
        const auto marker = (last && (i == packets_count));
        auto packet = _allocator.Allocate(tp, marker);
        auto header_size = packet.GetSize();

        auto payload_ptr = packet.GetView().ptr + header_size;
        payload_ptr[0] = CreateFuAIndicator(view.ptr[0]);
        payload_ptr[1] = CreateFuAHeader(i == 1, i == packets_count, view.ptr[0]);
        payload_ptr += 2;

        const auto packet_fua_payload_size = DivCeil(nalu_payload.size, packets_count + 1 - i);
        const auto chunk_size = std::min(nalu_payload.size, packet_fua_payload_size);
        memcpy(payload_ptr, nalu_payload.ptr, chunk_size);
        packet.SetSize(header_size + chunk_size);
        _callback(std::move(packet));

        nalu_payload.ForwardPtrUnsafe(chunk_size);
    }
}

uint8_t H264Packetizer::CreateFuAIndicator(uint8_t nalu_header) {
    return (nalu_header & 0b01100000) | h264::NaluType::kFuA;
}

uint8_t H264Packetizer::CreateFuAHeader(bool start, bool end, uint8_t nalu_header) {
    return (start       ? 0b10000000 : 0)
         | (end         ? 0b01000000 : 0)
         | (nalu_header & 0b00011111);
}

}
