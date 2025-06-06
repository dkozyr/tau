#include <tau/rtp-packetization/H265Depacketizer.h>
#include <tau/rtp-packetization/FuHeader.h>
#include <tau/rtp/Reader.h>
#include <tau/video/h265/Nalu.h>
#include <tau/common/NetToHost.h>
#include <cstring>
#include <numeric>

namespace tau::rtp {

using namespace h265;

H265Depacketizer::H265Depacketizer(Allocator& allocator)
    : _allocator(allocator)
{}

bool H265Depacketizer::Process(Frame&& frame) {
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

bool H265Depacketizer::Process(BufferViewConst rtp_payload_view, Timepoint tp, bool last) {
    if(rtp_payload_view.size < kNaluHeaderSize) {
        return false;
    }
    if(rtp_payload_view.ptr[0] & kNaluForbiddenMask) {
        return false;
    }
    auto type = GetNaluTypeUnsafe(rtp_payload_view.ptr);
    if(type == NaluType::kFu) {
        return ProcessFu(rtp_payload_view, tp, last);
    }
    _fu_nal_unit.reset();

    if(type == NaluType::kAp) {
        return ProcessAp(rtp_payload_view, tp, last);
    }
    if(type < NaluType::kAp) {
        return ProcessSingle(rtp_payload_view, tp, last);
    }
    return false;
}

bool H265Depacketizer::ProcessSingle(BufferViewConst rtp_payload_view, Timepoint tp, bool last) {
    auto nalu = Buffer::Create(_allocator, rtp_payload_view,
        Buffer::Info{.tp = tp, .flags = last ? kFlagsLast : kFlagsNone});
    _callback(std::move(nalu));
    return true;
}

bool H265Depacketizer::ProcessFu(BufferViewConst rtp_payload_view, Timepoint tp, bool last) {
    if(!ValidateFu(rtp_payload_view)) {
        _fu_nal_unit.reset();
        return false;
    }
    const auto fu_header = reinterpret_cast<const FuHeader*>(&rtp_payload_view.ptr[kNaluHeaderSize]);
    if(fu_header->start) {
        _fu_nal_unit.emplace(Buffer::Create(_allocator, _nalu_max_size, Buffer::Info{.tp = tp, .flags = kFlagsNone}));

        auto view = _fu_nal_unit->GetView();
        view.ptr[0] = rtp_payload_view.ptr[0];
        view.ptr[1] = rtp_payload_view.ptr[1];
        SetNaluHeaderTypeUnsafe(view.ptr, static_cast<NaluType>(fu_header->type));
        _fu_nal_unit->SetSize(kNaluHeaderSize);
    }
    rtp_payload_view.ForwardPtrUnsafe(kNaluHeaderSize + sizeof(FuHeader));

    auto nalu_view = _fu_nal_unit->GetView();
    nalu_view.ForwardPtrUnsafe(nalu_view.size);
    std::memcpy(nalu_view.ptr, rtp_payload_view.ptr, rtp_payload_view.size);
    _fu_nal_unit->SetSize(_fu_nal_unit->GetSize() + rtp_payload_view.size);

    if(fu_header->end) {
        if(last) {
            _fu_nal_unit->GetInfo().flags = kFlagsLast;
        }
        _callback(std::move(*_fu_nal_unit));
        _fu_nal_unit.reset();
    }
    return true;
}

bool H265Depacketizer::ProcessAp(BufferViewConst rtp_payload_view, Timepoint tp, bool last) {
    rtp_payload_view.ForwardPtrUnsafe(kNaluHeaderSize);
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

bool H265Depacketizer::ValidateFu(BufferViewConst rtp_payload_view) const {
    if(rtp_payload_view.size <= kNaluHeaderSize + sizeof(FuHeader)) {
        return false;
    }
    const auto fu_header = reinterpret_cast<const FuHeader*>(&rtp_payload_view.ptr[kNaluHeaderSize]);
    if(fu_header->start && fu_header->end) {
        return false;
    }
    if(!fu_header->start && !_fu_nal_unit.has_value()) {
        return false;
    }
    if(fu_header->type >= NaluType::kAp) {
        return false;
    }
    return true;
}

size_t H265Depacketizer::GetNaluMaxSize(const Frame& frame) {
    auto sum = std::accumulate(frame.begin(), frame.end(), 0, [](size_t total, const Buffer& packet) {
        return total + packet.GetSize();
    });
    return std::max(sum, kNaluMaxSizeDefault);
}

}
