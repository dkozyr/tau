#pragma once

#include "tau/memory/Allocator.h"
#include <cstdlib>

namespace tau {

class SystemAllocator : public Allocator {
public:
    static constexpr auto kDefaultSize = 0x1'0000;

public:
    ~SystemAllocator() = default;

    uint8_t* Allocate() override {
        return Allocate(kDefaultSize);
    }

    uint8_t* Allocate(size_t size) override {
        return static_cast<uint8_t*>(std::malloc(size));
    }

    void Deallocate(uint8_t* ptr) override {
        std::free(ptr);
    }

    size_t GetChunkSize() const {
        return kDefaultSize;
    }
};

inline SystemAllocator g_system_allocator;

}
