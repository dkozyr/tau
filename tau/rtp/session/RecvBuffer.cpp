#include "tau/rtp/session/RecvBuffer.h"
#include "tau/rtp/Sn.h"
#include <algorithm>

namespace rtp::session {

RecvBuffer::RecvBuffer(size_t size)
    : _size(std::clamp<uint16_t>(size, 16, 4096))
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
    if(_sn_next != _sn_end) {
        const auto expected_sn_end = SnForward(_sn_end, 1);
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

    if(InRange(sn, _sn_next, _sn_end)) {
        return OnInRangePacket(std::move(packet), sn);
    }

    const auto max_sn_end = SnForward(_sn_end, _size - 1);
    if(InRange(sn, _sn_end, max_sn_end)) {
        return OnOrderedPacket(std::move(packet), sn);
    }

    const auto min_sn_late = SnBackward(_sn_end, _size * 2);
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
        return PacketType::kOk;
    } else {
        _stats.discarded++;
        return PacketType::kDiscarded;
    }
}

RecvBuffer::PacketType RecvBuffer::OnOrderedPacket(Buffer&& packet, uint16_t sn) {
    while(SnDelta(_sn_end, _sn_next) + 1 >= _size) {
        SendAndProcessNext();
    }
    if(_sn_next == sn) {
        _callback(std::move(packet));
        IncreaseSnState();
    } else {
        const auto expected_sn_next = SnBackward(sn, _size);
        while(SnLesser(_sn_next, expected_sn_next)) {
            SendAndProcessNext();
        }
        _sn_end = sn;

        const auto index = GetIndexBySn(sn);
        _packets[index].emplace(std::move(packet));
    }
    return PacketType::kOk;
}

void RecvBuffer::DoReset(Buffer&& packet, uint16_t sn) {
    Flush();
    _callback(std::move(packet));
    _sn_next = SnForward(sn, 1);
    _sn_end = _sn_next;
}

void RecvBuffer::SendAndProcessNext() {
    if(_packets[_index].has_value()) {
        _callback(std::move(*_packets[_index]));
        _packets[_index].reset();
    } else {
        _stats.lost++;
    }
    IncreaseSnState();
}

void RecvBuffer::IncreaseSnState() {
    if(_sn_end == _sn_next) {
        _sn_end++;
    }
    _sn_next++;
    _index = (_index + 1) % _size;
}

size_t RecvBuffer::GetIndexBySn(uint16_t sn) const {
    const auto index = _index + SnDelta(sn, _sn_next);
    return index % _size;
}

}
