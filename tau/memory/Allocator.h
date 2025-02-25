#pragma once

#include <cstddef>
#include <cstdint>

class Allocator {
public:
    virtual ~Allocator() = default;

    virtual uint8_t* Allocate(size_t size) = 0;
    virtual void Deallocate(uint8_t* ptr) = 0;
};
