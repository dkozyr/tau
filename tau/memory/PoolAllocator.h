#pragma once

#include "tau/memory/Allocator.h"
#include "tau/common/Math.h"
#include <boost/pool/pool.hpp>
#include <mutex>

namespace tau {

inline constexpr size_t kUdpMtuSize = 1500;

class PoolAllocator : public Allocator {
public:
    explicit PoolAllocator(size_t block_size)
        : _pool(Align(block_size, sizeof(size_t)))
    {}

    ~PoolAllocator() = default;

    uint8_t* Allocate() override {
        std::lock_guard lock{_mutex};
        return reinterpret_cast<uint8_t*>(_pool.malloc());
    }

    uint8_t* Allocate(size_t size) override {
        if(size <= GetChunkSize()) {
            std::lock_guard lock{_mutex};
            return reinterpret_cast<uint8_t*>(_pool.malloc());
        } else {
            return nullptr;
        }
    }

    void Deallocate(uint8_t* ptr) override {
        std::lock_guard lock{_mutex};
        _pool.free(ptr);
    }

    size_t GetChunkSize() const override {
        return _pool.get_requested_size();
    }

private:
    mutable std::mutex _mutex;
    boost::pool<> _pool;
};

}
