#include <tau/rtp/H264Depacketizer.h>
#include <tau/rtp/H264Depacketizer.h>
#include <tau/rtp/Reader.h>
#include <tau/common/NetToHost.h>
#include <cstring>
#include <numeric>

namespace rtp {

using namespace h264;

H264Depacketizer::H264Depacketizer(Allocator& allocator)
    : _allocator(allocator)
{}

bool H264Depacketizer::Process(Frame&& frame) {
    bool ok = true;
    _nalu_max_size = GetNaluMaxSize(frame);
    for(const auto& packet : frame) {
        Reader reader(packet.GetView());
        ok &= Process(reader.Payload());
    }
    return ok;
}

bool H264Depacketizer::Process(BufferViewConst rtp_payload_view) {
    if(rtp_payload_view.size == 0) {
        return false;
    }
    const auto header = reinterpret_cast<const NaluHeader*>(&rtp_payload_view.ptr[0]);
    if(header->forbidden) {
        return false;
    }

    if(header->type == NaluType::kFuA) {
        return ProcessFuA(rtp_payload_view);
    }
    _fua_nal_unit.reset();

    if(header->type == NaluType::kStapA) {
        return ProcessStapA(rtp_payload_view);
    }
    if(header->type < NaluType::kStapA) {
        return ProcessSingle(rtp_payload_view);
    }
    return false;
}

bool H264Depacketizer::ProcessSingle(BufferViewConst rtp_payload_view) {
    auto nalu = Buffer::Create(_allocator, rtp_payload_view.size, Buffer::Info{.tp = 0});
    std::memcpy(nalu.GetView().ptr, rtp_payload_view.ptr, rtp_payload_view.size);
    nalu.SetSize(rtp_payload_view.size);
    _callback(std::move(nalu));
    return true;
}

bool H264Depacketizer::ProcessFuA(BufferViewConst rtp_payload_view) {
    if(!ValidateFuA(rtp_payload_view)) {
        _fua_nal_unit.reset();
        return false;
    }
    const auto fua_header = reinterpret_cast<const FuAHeader*>(&rtp_payload_view.ptr[1]);
    if(fua_header->start) {
        _fua_nal_unit.emplace(Buffer::Create(_allocator, _nalu_max_size, Buffer::Info{.tp = 0}));

        const auto fua_indicator = reinterpret_cast<const FuAIndicator*>(&rtp_payload_view.ptr[0]);
        _fua_nal_unit->GetView().ptr[0] = CreateNalUnitHeader(fua_header->type, fua_indicator->nri);
        _fua_nal_unit->SetSize(sizeof(NaluHeader));
    }
    rtp_payload_view.ForwardPtrUnsafe(sizeof(FuAIndicator) + sizeof(FuAHeader));

    auto nalu_view = _fua_nal_unit->GetView();
    nalu_view.ForwardPtrUnsafe(nalu_view.size);
    std::memcpy(nalu_view.ptr, rtp_payload_view.ptr, rtp_payload_view.size);
    _fua_nal_unit->SetSize(_fua_nal_unit->GetSize() + rtp_payload_view.size);

    if(fua_header->end) {
        _callback(std::move(*_fua_nal_unit));
        _fua_nal_unit.reset();
    }
    return true;
}

bool H264Depacketizer::ProcessStapA(BufferViewConst rtp_payload_view) {
    rtp_payload_view.ForwardPtrUnsafe(sizeof(NaluHeader));
    while(rtp_payload_view.size > sizeof(uint16_t)) {
        const auto nalu_size = Read16(rtp_payload_view.ptr);
        if((nalu_size == 0) || (rtp_payload_view.size < nalu_size)) {
            break;
        }
        rtp_payload_view.ForwardPtrUnsafe(sizeof(uint16_t));

        auto nalu = Buffer::Create(_allocator, nalu_size, Buffer::Info{.tp = 0});
        std::memcpy(nalu.GetView().ptr, rtp_payload_view.ptr, nalu_size);
        nalu.SetSize(nalu_size);
        _callback(std::move(nalu));
        rtp_payload_view.ForwardPtrUnsafe(nalu_size);
    }
    return (rtp_payload_view.size == 0);
}

bool H264Depacketizer::ValidateFuA(BufferViewConst rtp_payload_view) const {
    if(rtp_payload_view.size <= sizeof(FuAIndicator) + sizeof(FuAHeader)) {
        return false;
    }
    const auto fua_header = reinterpret_cast<const FuAHeader*>(&rtp_payload_view.ptr[1]);
    if(fua_header->start && fua_header->end) {
        return false;
    }
    if(!fua_header->start && !_fua_nal_unit.has_value()) {
        return false;
    }
    if(fua_header->type >= NaluType::kStapA) {
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
