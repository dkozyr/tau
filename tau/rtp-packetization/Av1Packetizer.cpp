#include <tau/rtp-packetization/Av1Packetizer.h>
#include <tau/rtp-packetization/Av1AggregationHeader.h>
#include <tau/rtp/Constants.h>
#include <tau/video/av1/Obu.h>
#include <tau/video/av1/Leb128.h>
#include <algorithm>
#include <cstring>
#include <vector>

namespace tau::rtp {

Av1Packetizer::Av1Packetizer(RtpAllocator& allocator)
    : _allocator(allocator)
    , _max_payload(allocator.MaxRtpPayload())
{}

bool Av1Packetizer::Process(const Buffer& au) {
    const auto parsed_units = ReadUnits(au.GetView());
    if(parsed_units.empty()) {
        return false;
    }
    return Process(parsed_units, au.GetInfo().tp, true, true);
}

bool Av1Packetizer::Process(const Buffer& input, bool last) {
    const auto input_view = input.GetView();
    const auto unit = av1::ReadObu(input_view);
    if(!unit || (unit->size != input_view.size)) {
        return false;
    }
    if(unit->Ignored()) {
        return true;
    }
    return Process({*unit}, input.GetInfo().tp, last, false);
}

bool Av1Packetizer::Process(const std::vector<av1::Obu>& units, Timepoint tp, bool last, bool aggregate) {
    const bool new_sequence = _new_sequence || (aggregate && (units.front().Type() == av1::ObuType::kSequenceHeader));
    size_t unit_index = 0;
    size_t unit_offset = 0;
    bool first_packet = true;
    while(unit_index < units.size()) {
        auto packet = _allocator.Allocate(tp, false);
        const auto header_size = packet.GetSize();
        auto payload = packet.GetView().ptr + header_size;
        payload[0] = (unit_offset ? kAv1ContinuationMask : 0) | (first_packet && new_sequence ? kAv1NewSequenceMask : 0);
        if(!aggregate) {
            payload[0] |= 1 << kAv1ElementCountShift;
        }
        const auto payload_size = WritePayload(payload, units, unit_index, unit_offset);
        if(last && (unit_index == units.size())) {
            packet.GetView().ptr[1] |= kMarkerMask;
        }
        packet.SetSize(header_size + payload_size);
        _callback(std::move(packet));
        first_packet = false;
    }
    _new_sequence = false;
    return true;
}

size_t Av1Packetizer::WritePayload(uint8_t* payload, const std::vector<av1::Obu>& units, size_t& unit_index, size_t& unit_offset) {
    const bool aggregate = !(payload[0] & kAv1ElementCountMask);
    size_t payload_size = kAv1AggregationHeaderSize;
    while(unit_index < units.size()) {
        const auto& unit = units[unit_index];
        const auto unit_size = unit.header.size + unit.payload.size;
        const auto available = _max_payload - payload_size;
        if(available <= 2) {
            break;
        }
        const auto reserved_size = aggregate ? av1::Leb128Size(available) : 0;
        size_t chunk_size = std::min(unit_size - unit_offset, available - reserved_size);
        if((unit_offset == 0) && (chunk_size < unit.header.size)) {
            break;
        }

        // Keep OBU boundary inside packet
        if((unit_offset + chunk_size == unit_size) && (unit_index + 1 < units.size())) {
            const auto next_header_size = units[unit_index + 1].header.size;
            const auto used = chunk_size + av1::Leb128Size(chunk_size);

            const bool next_unit_needs_more_space = (available - used < next_header_size + 2);
            const bool can_shorten_chunk = chunk_size > (unit_offset ? 1 : unit.header.size);
            if(next_unit_needs_more_space && can_shorten_chunk) {
                --chunk_size;
            }
        }

        if(aggregate) {
            std::array<uint8_t, 8> encoded_size;
            const auto size_length = av1::WriteLeb128(encoded_size.data(), chunk_size);
            memcpy(payload + payload_size, encoded_size.data(), size_length);
            payload_size += size_length;
        }

        for(size_t i = 0; i < chunk_size; ++i) {
            const auto offset = unit_offset + i;
            if(offset < unit.header.size) {
                payload[payload_size + i] = unit.header.ptr[offset];
            } else {
                payload[payload_size + i] = unit.payload.ptr[offset - unit.header.size];
            }
        }
        if(unit_offset == 0) {
            payload[payload_size] &= ~av1::kObuHasSizeMask;
        }
        payload_size += chunk_size;
        unit_offset += chunk_size;
        if(unit_offset < unit_size) {
            payload[0] |= kAv1ContinuesMask;
            break;
        }

        ++unit_index;
        unit_offset = 0;
    }
    return payload_size;
}

std::vector<av1::Obu> Av1Packetizer::ReadUnits(BufferViewConst view) {
    std::vector<av1::Obu> units;
    while(view.size > 0) {
        const auto unit = av1::ReadObu(view, true);
        if(!unit) {
            units.clear();
            break;
        }
        if(!unit->Ignored()) {
            units.push_back(*unit);
        }
        view.ForwardPtrUnsafe(unit->size);
    }
    return units;
}

}
