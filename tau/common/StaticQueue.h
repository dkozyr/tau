#pragma once

#include <tau/common/lock/Barrier.h>
#include <tau/common/lock/Cache.h>
#include <etl/array.h>
#include <optional>
#include <cstdint>
#include <cstddef>

namespace tau {

// lock-free single-producer single-consumer queue
template <typename T, size_t Capacity>
class StaticQueue {
public:
    static constexpr size_t kMask = Capacity - 1;

public:
    StaticQueue()
        : _head(0)
        , _tail(0)
    {
        static_assert((Capacity | kMask) == (Capacity + kMask)); // power of 2
    }

    bool Push(T&& element) {
        auto head = _head;
        auto next = (_head + 1) & kMask;
        if(next == _tail) {
            return false;
        }

        _buffer[head] = std::move(element);
        lock::BarrierRelease();
        _head = next;
        return true;
    }

    bool TryPop(T& output) {
        auto tail = _tail;
        if(tail == _head) {
            return false;
        }

        output = std::move(*_buffer[tail]);
        _buffer[tail].reset();
        lock::BarrierAcquire();
        _tail = (tail + 1) & kMask;
        return true;
    }

    bool Empty() const {
        return (_head == _tail);
    }

    bool Full() const {
        return (((_head + 1) & kMask) == _tail);
    }

    size_t Size() const {
        return (_head + Capacity - _tail) & kMask;
    }

private:
    etl::array<std::optional<T>, Capacity> _buffer;

    volatile size_t _head __attribute__((aligned(kCacheLine)));
    volatile size_t _tail __attribute__((aligned(kCacheLine)));
};

}
