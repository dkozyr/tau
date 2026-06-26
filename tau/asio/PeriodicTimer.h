#pragma once

#include "tau/asio/Timer.h"
#include <functional>

namespace tau {

class PeriodicTimer {
public:
    using Callback = std::function<bool(beast_ec ec)>;

public:
    PeriodicTimer(Executor executor)
        : _timer(executor)
    {}

    ~PeriodicTimer() {
        Stop();
    }

    void Start(size_t period_ms, Callback callback) {
        _period_ms = period_ms;
        _callback = std::move(callback);
        StartInternal();
    }

    void Stop() {
        _timer.cancel();
    }

private:
    void StartInternal() {
        _timer.expires_after(std::chrono::milliseconds(_period_ms));
        _timer.async_wait([this](boost_ec ec) {
            if(ec != boost::system::errc::operation_canceled) {
                OnTimer(ec);
            }
        });
    }

    void OnTimer(boost_ec ec) {
        if(_callback(ec)) {
            StartInternal();
        }
    }

private:
    Timer _timer;
    size_t _period_ms;
    Callback _callback;
};

}
