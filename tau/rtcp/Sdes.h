#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

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
    std::string_view data;
};
using SdesItems = std::vector<SdesItem>;

struct SdesChunk {
    uint32_t ssrc;
    SdesItems items;
};
using SdesChunks = std::vector<SdesChunk>;

}
