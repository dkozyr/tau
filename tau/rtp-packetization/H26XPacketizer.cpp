#include <tau/rtp-packetization/H26XPacketizer.h>
#include <tau/common/Math.h>
#include <cstring>

namespace tau::rtp {

H26XPacketizer::H26XPacketizer(RtpAllocator& allocator, Options&& options)
    : _allocator(allocator)
    , _options(std::move(options))
    , _max_payload(_allocator.MaxRtpPayload())
{}

bool H26XPacketizer::Process(const Buffer& nal_unit, bool last) {
    auto view = nal_unit.GetView();
    if(!_options.validate_header(view)) {
        return false;
    }

    const auto tp = nal_unit.GetInfo().tp;
    if(view.size <= _max_payload) {
        ProcessSingle(view, tp, last);
    } else {
        ProcessFragmented(view, tp, last);
    }
    return true;
}

void H26XPacketizer::ProcessSingle(const BufferViewConst& view, Timepoint tp, bool last) {
    auto packet = _allocator.Allocate(tp, last);
    auto header_size = packet.GetSize();
    auto payload_ptr = packet.GetView().ptr + header_size;
    memcpy(payload_ptr, view.ptr, view.size);
    packet.SetSize(header_size + view.size);
    _callback(std::move(packet));
}

void H26XPacketizer::ProcessFragmented(const BufferViewConst& view, Timepoint tp, bool last) {
    auto nalu_payload = view;
    nalu_payload.ForwardPtrUnsafe(_options.nalu_header_size);

    const auto nalu_type = _options.get_nalu_type(view);
    const auto nalu_headers_size = _options.nalu_header_size + sizeof(uint8_t);
    const auto max_fua_payload = _max_payload - nalu_headers_size;
    const auto packets_count = DivCeil(nalu_payload.size, max_fua_payload);

    for(size_t i = 1; i <= packets_count; ++i) {
        const auto marker = (last && (i == packets_count));
        auto packet = _allocator.Allocate(tp, marker);
        auto rtp_header_size = packet.GetSize();

        auto payload_ptr = packet.GetView().ptr + rtp_header_size;
        std::memcpy(payload_ptr, view.ptr, _options.nalu_header_size);
        _options.set_nalu_type(payload_ptr, _options.fragmented_nalu_type);
        payload_ptr += _options.nalu_header_size;
        payload_ptr[0] = CreateFragmentedHeader(i == 1, i == packets_count, nalu_type);
        payload_ptr += sizeof(uint8_t);

        const auto packet_fua_payload_size = DivCeil(nalu_payload.size, packets_count + 1 - i);
        const auto chunk_size = std::min(nalu_payload.size, packet_fua_payload_size);
        memcpy(payload_ptr, nalu_payload.ptr, chunk_size);
        packet.SetSize(rtp_header_size + nalu_headers_size + chunk_size);
        _callback(std::move(packet));

        nalu_payload.ForwardPtrUnsafe(chunk_size);
    }
}

uint8_t H26XPacketizer::CreateFragmentedHeader(bool start, bool end, uint8_t type) {
    return (start ? 0b10000000 : 0)
         | (end   ? 0b01000000 : 0)
         | (type  & 0b00111111);
}

}
