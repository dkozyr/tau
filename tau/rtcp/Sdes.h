#pragma once

#include <etl/string_view.h>
#include <etl/vector.h>
#include <cstdint>

namespace tau::rtcp {

enum SdesType : uint8_t {
    kEnd   = 0,
    kCname = 1,
    kName  = 2,
    kEmail = 3,
    kPhone = 4,
    kLoc   = 5,
    kTool  = 6,
    kNote  = 7,
    kPriv  = 8,
};

struct SdesItem {
    SdesType type;
    etl::string_view data;
};
using SdesItems = etl::vector<SdesItem, 4>;

struct SdesChunk {
    uint32_t ssrc;
    SdesItems items;
};
using SdesChunks = etl::vector<SdesChunk, 4>;

}
