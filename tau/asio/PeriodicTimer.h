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
        beast_ec ec;
        _timer.cancel(ec);
    }

private:
    void StartInternal() {
        _timer.expires_after(std::chrono::milliseconds(_period_ms));
        _timer.async_wait([this](beast_ec ec) {
            OnTimer(ec);
        });
    }

    void OnTimer(beast_ec ec) {
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
