#pragma once

#include <tau/memory/Allocator.h>
#include <tau/common/Math.h>
#include <mutex>

namespace tau {

template<typename TIndex = uint16_t>
class PoolAllocator : public Allocator {
public:
    explicit PoolAllocator(void* ptr, size_t allocated_size, size_t block_size)
        : _block_size(Align(block_size, sizeof(size_t)))
        , _max_block_count(static_cast<TIndex>(allocated_size / (_block_size + sizeof(TIndex))))
        , _ptr(reinterpret_cast<uint8_t*>(ptr))
        , _free_block_index(reinterpret_cast<TIndex*>(_ptr + _max_block_count * _block_size)) {
        for(TIndex i = 0; i < _max_block_count; ++i) {
            _free_block_index[i] = i;
        }
    }

    uint8_t* Allocate() override {
        std::lock_guard lock{_mutex};
        if(_used_block_count < _max_block_count) {
            auto index = _free_block_index[_used_block_count];
            _used_block_count++;
            return reinterpret_cast<uint8_t*>(_ptr + index * _block_size);
        } else {
            return nullptr; //TODO: abort? debug_assert?
        }
    }

    uint8_t* Allocate(size_t size) override {
        if(size <= GetChunkSize()) {
            return Allocate();
        } else {
            return nullptr;
        }
    }

    void Deallocate(uint8_t* ptr) override {
        std::lock_guard lock{_mutex};
        auto index = static_cast<TIndex>((ptr - _ptr) / _block_size);
        _used_block_count--;
        _free_block_index[_used_block_count] = index;
    }

    size_t GetChunkSize() const override {
        return _block_size;
    }

    size_t GetMaxBlockCount() const {
        return static_cast<size_t>(_max_block_count);
    }

    size_t GetFreeBlockCount() const {
        //TODO: debug assert?
        return _max_block_count - _used_block_count;
    }

private:
    mutable std::mutex _mutex;
    const size_t _block_size;
    const TIndex _max_block_count;
    uint8_t* _ptr;

    TIndex* _free_block_index;
    size_t _used_block_count = 0;
};

}
