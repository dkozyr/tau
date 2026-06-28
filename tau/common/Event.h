#pragma once

#include <atomic>
#include <future>
#include <chrono>

namespace tau {

//TODO: EventTest
class Event {
public:
    Event()
        : _future(_done.get_future())
    {}

    void Set() {
        if(!_is_set.exchange(true)) {
            _done.set_value();
        }
    }

    bool IsSet() const {
        return _is_set.load();
    }

    template<typename Rep, typename Period>
    bool WaitFor(const std::chrono::duration<Rep, Period>& timeout) {
        return _future.wait_for(timeout) == std::future_status::ready;
    }

private:
    std::promise<void> _done;
    std::future<void> _future;
    std::atomic<bool> _is_set{false};
};

}
