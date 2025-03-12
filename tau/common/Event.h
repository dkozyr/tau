#pragma once

#include <future>
#include <chrono>

class Event {
public:
    void Set();
    bool IsSet();

    template<typename Rep, typename Period>
    bool WaitFor(const std::chrono::duration<Rep, Period>& timeout) {
        return std::future_status::ready == _done.get_future().wait_for(timeout);
    }

private:
    std::promise<void> _done;
};
