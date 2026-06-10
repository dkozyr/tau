#pragma once

#include "tau/rtp/Frame.h"
#include "tau/rtp/Reader.h"
#include "tau/rtp/Sn.h"
#include <functional>
#include <optional>

namespace tau::rtp::session {

// Group RTP packets to Frames by TS, detect losses by SN
class FrameProcessor {
public:
    using Callback = std::function<void(Frame&& frame, bool losses)>;

public:
    void SetCallback(Callback callback) { _callback = std::move(callback); }

    void PushRtp(Buffer&& packet) {
        Reader reader(ToConst(packet.GetView()));
        const auto ts = reader.Ts();
        const auto sn = reader.Sn();
        if(_sn_next && (*_sn_next != sn)) {
            _losses = true;
        }
        if(_ts && (*_ts != ts)) {
            DoCallback();
        }
        _ts = ts;
        _sn_next = SnForward(sn, 1);
        _frame.push_back(std::move(packet));
        if(reader.Marker()) {
            DoCallback();
        }
    }

private:
    void DoCallback() {
        _callback(std::move(_frame), _losses);
        _ts.reset();
        _losses = false;
        _frame.clear();
    }

private:
    std::optional<uint32_t> _ts;
    std::optional<uint16_t> _sn_next;
    bool _losses = false;
    Frame _frame;
    Callback _callback;
};

}
