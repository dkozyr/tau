#include "tau/sdp/Media.h"
#include <algorithm>

namespace tau::sdp {

std::vector<uint8_t> GetPtOrdered(const CodecsMap& codecs) {
    auto pt_with_priority = GetPtWithPriority(codecs);
    std::vector<uint8_t> pts;
    pts.reserve(pt_with_priority.size());
    for(auto& pt : pt_with_priority) {
        pts.push_back(pt.pt);
    }
    return pts;
}

std::vector<PtWithPriority> GetPtWithPriority(const CodecsMap& codecs) {
    std::vector<PtWithPriority> pts;
    pts.reserve(codecs.size());
    for(auto& [pt, codec] : codecs) {
        pts.push_back(PtWithPriority{.pt = pt, .index = codec.index});
    }
    std::sort(pts.begin(), pts.end(), [](const PtWithPriority& a, const PtWithPriority& b) {
        return a.index < b.index;
    });
    return pts;
}

}
