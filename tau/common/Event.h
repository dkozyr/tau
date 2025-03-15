#pragma once

#include <future>
#include <chrono>

namespace tau {

class Event {
public:
    void Set() {
        _done.set_value();
    }

    bool IsSet() {
        return _done.get_future().valid();
    }

    template<typename Rep, typename Period>
    bool WaitFor(const std::chrono::duration<Rep, Period>& timeout) {
        return std::future_status::ready == _done.get_future().wait_for(timeout);
    }

private:
    std::promise<void> _done;
};

}
