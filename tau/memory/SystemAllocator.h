#pragma once

#include "tau/memory/Allocator.h"
#include <cstdlib>

class SystemAllocator : public Allocator {
public:
    ~SystemAllocator() = default;

    uint8_t* Allocate(size_t size) override {
        return static_cast<uint8_t*>(std::malloc(size));
    }

    void Deallocate(uint8_t* ptr) override {
        std::free(ptr);
    }
};

inline SystemAllocator g_system_allocator;
