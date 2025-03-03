#include "tau/rtp/session/RecvBuffer.h"
#include "tau/rtp/Sn.h"
#include "tau/common/Container.h"
#include <algorithm>

namespace rtp::session {

RecvBuffer::RecvBuffer(size_t size)
    : _size(std::clamp<size_t>(size, 4, 4096))
    , _packets(_size) {
}

RecvBuffer::PacketType RecvBuffer::Push(Buffer&& packet, uint16_t sn) {
    const auto result = InsertPacket(std::move(packet), sn);
    while(_packets[_index].has_value()) {
        SendAndProcessNext();
    }
    return result;
}

void RecvBuffer::Flush() {
    if(_sn_end.has_value()) {
        const auto expected_sn_end = SnForward(*_sn_end, 1);
        while(_sn_next != expected_sn_end) {
            SendAndProcessNext();
        }
    }
}

RecvBuffer::PacketType RecvBuffer::InsertPacket(Buffer&& packet, uint16_t sn) {
    _stats.packets++;
    _stats.bytes += packet.GetSize();

    if(_first_packet) {
        _first_packet = false;
        DoReset(std::move(packet), sn);
        return PacketType::kOk;
    }

    const auto sn_end = _sn_end.value_or(_sn_next);
    if(InRange(sn, _sn_next, sn_end)) {
        return OnInRangePacket(std::move(packet), sn);
    }

    const auto max_sn_end = SnForward(sn_end, _size - 1);
    if(InRange(sn, sn_end, max_sn_end)) {
        return OnOrderedPacket(std::move(packet), sn);
    }

    const auto min_sn_late = SnBackward(sn_end, _size * 2);
    if(InRange(sn, min_sn_late, _sn_next)) {
        _stats.discarded++;
        return PacketType::kDiscarded;
    }

    DoReset(std::move(packet), sn);
    return PacketType::kReset;
}

RecvBuffer::PacketType RecvBuffer::OnInRangePacket(Buffer&& packet, uint16_t sn) {
    const auto index = GetIndexBySn(sn);
    if(!_packets[index].has_value()) {
        _packets[index].emplace(std::move(packet));
        _sns_to_recover.erase(sn);
        return PacketType::kOk;
    } else {
        _stats.discarded++;
        return PacketType::kDiscarded;
    }
}

RecvBuffer::PacketType RecvBuffer::OnOrderedPacket(Buffer&& packet, uint16_t sn) {
    const auto expected_sn_next = SnBackward(sn, _size - 1);
    while(SnLesser(_sn_next, expected_sn_next)) {
        SendAndProcessNext();
    }

    const auto sn_begin_to_recover = _sn_end ? SnForward(*_sn_end, 1) : _sn_next;
    for(uint16_t sn_to_recover = sn_begin_to_recover; sn_to_recover != sn; ++sn_to_recover) {
        _sns_to_recover.insert(sn_to_recover);
    }

    const auto index = GetIndexBySn(sn);
    _packets[index].emplace(std::move(packet));
    _sn_end = sn;
    return PacketType::kOk;
}

void RecvBuffer::DoReset(Buffer&& packet, uint16_t sn) {
    Flush();
    _callback(std::move(packet));
    _sn_next = SnForward(sn, 1);
    _sn_end.reset();
}

void RecvBuffer::SendAndProcessNext() {
    if(_packets[_index].has_value()) {
        _callback(std::move(*_packets[_index]));
        _packets[_index].reset();
    } else {
        _sns_to_recover.erase(_sn_next);
        _stats.lost++;
    }
    IncreaseSnState();
}

void RecvBuffer::IncreaseSnState() {
    if(_sn_end && (_sn_next == *_sn_end)) {
        _sn_end.reset();
    }
    _sn_next++;
    _index = (_index + 1) % _size;
}

size_t RecvBuffer::GetIndexBySn(uint16_t sn) const {
    const auto index = _index + SnDelta(sn, _sn_next);
    return index % _size;
}

}
