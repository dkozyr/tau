#include <tau/rtp-packetization/H265Packetizer.h>
#include <tau/rtp-packetization/FuHeader.h>
#include <tau/video/h265/Nalu.h>
#include <tau/common/Math.h>
#include <cstring>

namespace tau::rtp {

using namespace h265;

H265Packetizer::H265Packetizer(RtpAllocator& allocator)
    : _allocator(allocator)
    , _max_payload(_allocator.MaxRtpPayload())
{}

bool H265Packetizer::Process(const Buffer& nal_unit, bool last) {
    auto view = nal_unit.GetView();
    if(view.size <= kNaluHeaderSize) {
        return false;
    }
    if(view.ptr[0] & kNaluForbiddenMask) {
        return false;
    }
    auto type = GetNaluTypeUnsafe(view.ptr);
    if(type >= NaluType::kAp) {
        return false;
    }

    const auto tp = nal_unit.GetInfo().tp;
    if(view.size <= _max_payload) {
        ProcessSingle(view, tp, last);
    } else {
        ProcessFu(view, tp, last);
    }
    return true;
}

void H265Packetizer::ProcessSingle(const BufferViewConst& view, Timepoint tp, bool last) {
    auto packet = _allocator.Allocate(tp, last);
    auto header_size = packet.GetSize();
    auto payload_ptr = packet.GetView().ptr + header_size;
    memcpy(payload_ptr, view.ptr, view.size);
    packet.SetSize(header_size + view.size);
    _callback(std::move(packet));
}

void H265Packetizer::ProcessFu(const BufferViewConst& view, Timepoint tp, bool last) {
    auto nalu_payload = view;
    nalu_payload.ForwardPtrUnsafe(kNaluHeaderSize);

    const auto max_fua_payload = _max_payload - kNaluHeaderSize - sizeof(FuHeader);
    const auto packets_count = DivCeil(nalu_payload.size, max_fua_payload);
    const auto nalu_type = GetNaluTypeUnsafe(view.ptr);

    for(size_t i = 1; i <= packets_count; ++i) {
        const auto marker = (last && (i == packets_count));
        auto packet = _allocator.Allocate(tp, marker);
        auto rtp_header_size = packet.GetSize();

        auto payload_ptr = packet.GetView().ptr + rtp_header_size;
        payload_ptr[0] = view.ptr[0];
        payload_ptr[1] = view.ptr[1];
        SetNaluHeaderTypeUnsafe(payload_ptr, NaluType::kFu);
        payload_ptr[2] = CreateFuHeader(i == 1, i == packets_count, static_cast<uint8_t>(nalu_type));
        payload_ptr += kNaluHeaderSize + sizeof(FuHeader);

        const auto packet_fua_payload_size = DivCeil(nalu_payload.size, packets_count + 1 - i);
        const auto chunk_size = std::min(nalu_payload.size, packet_fua_payload_size);
        memcpy(payload_ptr, nalu_payload.ptr, chunk_size);
        packet.SetSize(rtp_header_size + kNaluHeaderSize + sizeof(FuHeader) + chunk_size);
        _callback(std::move(packet));

        nalu_payload.ForwardPtrUnsafe(chunk_size);
    }
}

}
