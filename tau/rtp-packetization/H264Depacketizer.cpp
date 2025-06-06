#include <tau/rtp-packetization/H264Depacketizer.h>
#include <tau/rtp-packetization/FuHeader.h>
#include <tau/rtp/Reader.h>
#include <tau/video/h264/Nalu.h>
#include <tau/common/NetToHost.h>
#include <cstring>
#include <numeric>

namespace tau::rtp {

using namespace h264;

H264Depacketizer::H264Depacketizer(Allocator& allocator)
    : _allocator(allocator)
{}

bool H264Depacketizer::Process(Frame&& frame) {
    bool ok = true;
    _nalu_max_size = GetNaluMaxSize(frame);
    for(size_t i = 0; i < frame.size(); ++i) {
        const auto& packet = frame[i];
        Reader reader(packet.GetView());
        const auto last = reader.Marker() || ((i + 1) == frame.size());
        ok &= Process(reader.Payload(), packet.GetInfo().tp, last);
    }
    return ok;
}

bool H264Depacketizer::Process(BufferViewConst rtp_payload_view, Timepoint tp, bool last) {
    if(rtp_payload_view.size == 0) {
        return false;
    }
    const auto header = reinterpret_cast<const NaluHeader*>(&rtp_payload_view.ptr[0]);
    if(header->forbidden) {
        return false;
    }

    if(header->type == NaluType::kFuA) {
        return ProcessFuA(rtp_payload_view, tp, last);
    }
    _fua_nal_unit.reset();

    if(header->type == NaluType::kStapA) {
        return ProcessStapA(rtp_payload_view, tp, last);
    }
    if(header->type < NaluType::kStapA) {
        return ProcessSingle(rtp_payload_view, tp, last);
    }
    return false;
}

bool H264Depacketizer::ProcessSingle(BufferViewConst rtp_payload_view, Timepoint tp, bool last) {
    auto nalu = Buffer::Create(_allocator, rtp_payload_view,
        Buffer::Info{.tp = tp, .flags = last ? kFlagsLast : kFlagsNone});
    _callback(std::move(nalu));
    return true;
}

bool H264Depacketizer::ProcessFuA(BufferViewConst rtp_payload_view, Timepoint tp, bool last) {
    if(!ValidateFuA(rtp_payload_view)) {
        _fua_nal_unit.reset();
        return false;
    }
    const auto fu_header = reinterpret_cast<const FuHeader*>(&rtp_payload_view.ptr[1]);
    if(fu_header->start) {
        _fua_nal_unit.emplace(Buffer::Create(_allocator, _nalu_max_size, Buffer::Info{.tp = tp, .flags = kFlagsNone}));

        const auto fua_indicator = reinterpret_cast<const FuAIndicator*>(&rtp_payload_view.ptr[0]);
        _fua_nal_unit->GetView().ptr[0] = CreateNalUnitHeader(fu_header->type, fua_indicator->nri);
        _fua_nal_unit->SetSize(sizeof(NaluHeader));
    }
    rtp_payload_view.ForwardPtrUnsafe(sizeof(FuAIndicator) + sizeof(FuHeader));

    auto nalu_view = _fua_nal_unit->GetView();
    nalu_view.ForwardPtrUnsafe(nalu_view.size);
    std::memcpy(nalu_view.ptr, rtp_payload_view.ptr, rtp_payload_view.size);
    _fua_nal_unit->SetSize(_fua_nal_unit->GetSize() + rtp_payload_view.size);

    if(fu_header->end) {
        if(last) {
            _fua_nal_unit->GetInfo().flags = kFlagsLast;
        }
        _callback(std::move(*_fua_nal_unit));
        _fua_nal_unit.reset();
    }
    return true;
}

bool H264Depacketizer::ProcessStapA(BufferViewConst rtp_payload_view, Timepoint tp, bool last) {
    rtp_payload_view.ForwardPtrUnsafe(sizeof(NaluHeader));
    while(rtp_payload_view.size > sizeof(uint16_t)) {
        const auto nalu_size = Read16(rtp_payload_view.ptr);
        if((nalu_size == 0) || (rtp_payload_view.size < nalu_size)) {
            break;
        }
        rtp_payload_view.ForwardPtrUnsafe(sizeof(uint16_t));

        auto nalu = Buffer::Create(_allocator, {rtp_payload_view.ptr, nalu_size}, Buffer::Info{.tp = tp});
        rtp_payload_view.ForwardPtrUnsafe(nalu_size);
        if(last && (rtp_payload_view.size == 0)) {
            nalu.GetInfo().flags = kFlagsLast;
        }
        _callback(std::move(nalu));
    }
    return (rtp_payload_view.size == 0);
}

bool H264Depacketizer::ValidateFuA(BufferViewConst rtp_payload_view) const {
    if(rtp_payload_view.size <= sizeof(FuAIndicator) + sizeof(FuHeader)) {
        return false;
    }
    const auto fu_header = reinterpret_cast<const FuHeader*>(&rtp_payload_view.ptr[1]);
    if(fu_header->start && fu_header->end) {
        return false;
    }
    if(!fu_header->start && !_fua_nal_unit.has_value()) {
        return false;
    }
    if(fu_header->type >= NaluType::kStapA) {
        return false;
    }
    return true;
}

size_t H264Depacketizer::GetNaluMaxSize(const Frame& frame) {
    auto sum = std::accumulate(frame.begin(), frame.end(), 0, [](size_t total, const Buffer& packet) {
        return total + packet.GetSize();
    });
    return std::max(sum, kNaluMaxSizeDefault);
}

}
