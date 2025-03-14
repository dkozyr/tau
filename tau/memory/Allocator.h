#pragma once

#include <cstddef>
#include <cstdint>

namespace tau {

class Allocator {
public:
    virtual ~Allocator() = default;

    virtual uint8_t* Allocate() = 0;
    virtual uint8_t* Allocate(size_t size) = 0;
    virtual void Deallocate(uint8_t* ptr) = 0;

    virtual size_t GetChunkSize() const = 0;
};

}
