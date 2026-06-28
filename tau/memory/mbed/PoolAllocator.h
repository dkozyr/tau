#pragma once

#include <tau/memory/Allocator.h>
#include <tau/common/Math.h>
#include <tau/common/Log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

namespace tau {

class PoolAllocator : public Allocator {
    struct QueueItem {
        void* block;
    };

public:
    explicit PoolAllocator(void* ptr, size_t allocated_size, size_t block_size)
        : _block_size(Align(block_size, sizeof(size_t)))
        , _max_block_count(allocated_size / _block_size)
        , _ptr(reinterpret_cast<uint8_t*>(ptr))
        , _queue(xQueueCreate(_max_block_count, sizeof(void*))) {
        for(size_t i = 0; i < _max_block_count; ++i) {
            void* block = ptr;
            xQueueSend(_queue, &block, portMAX_DELAY);
            ptr += _block_size;
        }
    }

    ~PoolAllocator() {
        vQueueDelete(_queue);
    }

    uint8_t* Allocate() override {
        if(_used_block_count < _max_block_count) {
            _used_block_count++;
            void* block = nullptr;
            if(pdTRUE == xQueueReceive(_queue, &block, portMAX_DELAY)) {
                return reinterpret_cast<uint8_t*>(block);
            } else {
                TAU_LOG_ERROR("xQueueReceive failed, no memory block");
                return nullptr;
            }
        } else {
            TAU_LOG_ERROR("No free blocks, used_block_count: " << _used_block_count);
            return nullptr;
        }
    }

    uint8_t* Allocate(size_t size) override {
        if(size <= GetChunkSize()) {
            return Allocate();
        } else {
            TAU_LOG_ERROR("Too big block size: " << size);
            return nullptr;
        }
    }

    void Deallocate(uint8_t* ptr) override {
        _used_block_count--;
        void* block = reinterpret_cast<void*>(ptr);
        xQueueSend(_queue, &block, portMAX_DELAY);
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
    const size_t _block_size;
    const size_t _max_block_count;
    uint8_t* _ptr;

    QueueHandle_t _queue;
    size_t _used_block_count = 0;
};

}
