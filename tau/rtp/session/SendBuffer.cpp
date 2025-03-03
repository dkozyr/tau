#include "tau/rtp/session/SendBuffer.h"
#include "tau/rtp/Sn.h"

namespace rtp::session {

SendBuffer::SendBuffer(size_t size)
    : _size(std::clamp<size_t>(size, 4, 4096)) {
    _packets.reserve(_size);
}

void SendBuffer::Push(Buffer&& packet, uint16_t sn) {
    if(_packets.size() == _size) {
        std::swap(_packets.at(_index), packet);
        _sn_begin++;
        _index = (_index + 1) % _size;
    } else {
        if(_packets.empty()) {
            _sn_begin = sn;
            _index = 0;
        }
        _packets.push_back(std::move(packet));
    }

    const auto index = GetIndexBySn(sn);
    auto packet_copy = _packets.at(index).MakeCopy();
    _callback(std::move(packet_copy));

    _stats.packets++;
    _stats.bytes += _packets.at(index).GetSize();
}

bool SendBuffer::SendRtx(uint16_t sn) {
    if(_packets.empty()) {
        return false;
    }
    const auto sn_end = SnForward(_sn_begin, _packets.size() - 1);
    if(!InRange(sn, _sn_begin, sn_end)) {
        return false;
    }

    const auto index = GetIndexBySn(sn);
    auto packet_copy = _packets.at(index).MakeCopy();
    _callback(std::move(packet_copy));

    _stats.packets++;
    _stats.rtx++;
    _stats.bytes += _packets.at(index).GetSize();
    return true;
}

size_t SendBuffer::GetIndexBySn(uint16_t sn) const {
    const auto index = _index + SnDelta(sn, _sn_begin);
    return index % _packets.size();
}

}
