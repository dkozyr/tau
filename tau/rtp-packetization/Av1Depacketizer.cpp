#include <tau/rtp-packetization/Av1Depacketizer.h>
#include <tau/rtp-packetization/Av1AggregationHeader.h>
#include <tau/video/av1/Obu.h>
#include <tau/video/av1/Leb128.h>
#include <tau/rtp/Reader.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <utility>

namespace tau::rtp {

Av1Depacketizer::Av1Depacketizer(Allocator& allocator, size_t max_unit_size)
    : _allocator(allocator)
    , _max_unit_size(max_unit_size)
{}

bool Av1Depacketizer::Process(Frame&& frame) {
    bool ok = true;
    for(size_t i = 0; ok && (i < frame.size()); ++i) {
        const auto last = (i + 1 == frame.size());
        ok &= Process(frame[i], last);
    }
    return ok;
}

bool Av1Depacketizer::Process(const Buffer& packet, bool last) {
    Reader reader(packet.GetView());
    const PacketMetadata metadata{reader.Ssrc(), reader.Ts(), reader.Sn()};
    const bool marker = reader.Marker();
    last = last || marker;

    const bool continuous = _expected_packet && (metadata == *_expected_packet);
    _expected_packet = metadata;
    _expected_packet->sn++;
    if(!continuous) {
        _fragment.reset();
    }

    return ProcessPayload(reader.Payload(), packet.GetInfo().tp, last, marker);
}

bool Av1Depacketizer::ProcessPayload(BufferViewConst payload, Timepoint tp, bool last, bool marker) {
    if(payload.size <= kAv1AggregationHeaderSize) {
        return Fail();
    }

    const auto header = payload.ptr[0];
    const bool continuation = header & kAv1ContinuationMask;
    const bool continues    = header & kAv1ContinuesMask;
    const size_t count      = (header & kAv1ElementCountMask) >> kAv1ElementCountShift;
    const bool new_sequence = header & kAv1NewSequenceMask;
    if((continuation && new_sequence) || (continues && marker)) {
        return Fail();
    }
    if(!continuation) {
        _fragment.reset();
    }
    if(continuation && !_fragment) {
        return Fail();
    }
    payload.ForwardPtrUnsafe(kAv1AggregationHeaderSize);

    size_t i = 0;
    std::optional<Buffer> pending;
    while(payload.size > 0) {
        ++i;
        const auto element_size = ((count == 0) || (i < count))
            ? av1::ReadLeb128(payload)
            : std::optional<size_t>{payload.size};
        if(!element_size || (*element_size == 0) || (*element_size > payload.size) || (count && (i > count))) {
            return Fail();
        }

        const BufferViewConst element{payload.ptr, *element_size};
        payload.ForwardPtrUnsafe(*element_size);

        const bool final_element = (payload.size == 0);
        if(final_element && count && (i != count)) {
            return Fail();
        }

        if(!ProcessElement(element, tp, (i == 1) && continuation, final_element && continues, pending)) {
            return Fail();
        }
    }
    if(pending) {
        pending->GetInfo().flags = (last && !continues) ? kFlagsLast : kFlagsNone;
        _callback(std::move(*pending));
    }
    return true;
}

bool Av1Depacketizer::ProcessElement(BufferViewConst element, Timepoint tp, bool continuation, bool incomplete, std::optional<Buffer>& pending) {
    if(continuation) {
        if(!Append(element, tp)) {
            return false;
        }
        if(!incomplete) {
            if(!Complete(ToConst(_fragment->GetView()), _fragment->GetInfo().tp, pending)) {
                return false;
            }
            _fragment.reset();
        }
        return true;
    }
    if(incomplete) {
        return Append(element, tp);
    }
    return Complete(element, tp, pending);
}

bool Av1Depacketizer::Append(BufferViewConst element, Timepoint tp) {
    const auto current_size = _fragment ? _fragment->GetSize() : 0;
    if((current_size > _max_unit_size) || (element.size > _max_unit_size - current_size)) {
        return false;
    }

    const auto required_size = current_size + element.size;
    if(!_fragment || (required_size > _fragment->GetCapacity())) {
        const auto initial_capacity = std::min<size_t>(_max_unit_size, 4096);
        const auto growth_capacity = current_size > _max_unit_size / 2 ? _max_unit_size : current_size * 2;
        const auto capacity = std::max(required_size, std::max(initial_capacity, growth_capacity));
        auto replacement = Buffer::Create(_allocator, capacity, Buffer::Info{.tp = _fragment ? _fragment->GetInfo().tp : tp});
        if(_fragment) {
            memcpy(replacement.GetView().ptr, _fragment->GetView().ptr, current_size);
        }
        replacement.SetSize(current_size);
        _fragment.emplace(std::move(replacement));
    }
    memcpy(_fragment->GetView().ptr + current_size, element.ptr, element.size);
    _fragment->SetSize(required_size);
    return true;
}

bool Av1Depacketizer::Complete(BufferViewConst element, Timepoint tp, std::optional<Buffer>& pending) {
    if(element.size > _max_unit_size) {
        return false;
    }
    const auto unit = av1::ReadObu(element);
    if(!unit || (unit->size != element.size)) {
        return false;
    }
    if(unit->Ignored()) {
        return true;
    }

    std::array<uint8_t, 8> encoded_size;
    const auto size_length = av1::WriteLeb128(encoded_size.data(), unit->payload.size);
    auto buffer = Buffer::Create(_allocator, unit->header.size + size_length + unit->payload.size, Buffer::Info{.tp = tp});
    auto destination = buffer.GetView().ptr;
    memcpy(destination, unit->header.ptr, unit->header.size);

    destination[0] |= 0x02;
    memcpy(destination + unit->header.size, encoded_size.data(), size_length);
    memcpy(destination + unit->header.size + size_length, unit->payload.ptr, unit->payload.size);

    buffer.SetSize(unit->header.size + size_length + unit->payload.size);
    if(pending) {
        _callback(std::move(*pending));
    }
    pending.emplace(std::move(buffer));
    return true;
}

bool Av1Depacketizer::Fail() {
    _fragment.reset();
    return false;
}

}
