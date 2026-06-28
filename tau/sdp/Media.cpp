#include "tau/sdp/Media.h"
#include <algorithm>

namespace tau::sdp {

etl::vector<uint8_t, kMaxCodecs> GetPtOrdered(const CodecsMap& codecs) {
    auto pt_with_priority = GetPtWithPriority(codecs);
    etl::vector<uint8_t, kMaxCodecs> pts;
    for(auto& pt : pt_with_priority) {
        if(pts.full()) {
            break;
        }
        pts.push_back(pt.pt);
    }
    return pts;
}

etl::vector<PtWithPriority, kMaxCodecs> GetPtWithPriority(const CodecsMap& codecs) {
    etl::vector<PtWithPriority, kMaxCodecs> pts;
    for(auto& [pt, codec] : codecs) {
        if(pts.full()) {
            break;
        }
        pts.push_back(PtWithPriority{.pt = pt, .index = codec.index});
    }
    std::sort(pts.begin(), pts.end(), [](const PtWithPriority& a, const PtWithPriority& b) {
        return a.index < b.index;
    });
    return pts;
}

CodecsMap MakeCodecsMap(std::initializer_list<std::pair<const uint8_t, Codec>> list) {
    CodecsMap result;
    for(auto&& p : list) {
        if(result.full()) {
            break;
        }
        result[p.first] = std::move(p.second);
    }
    return result;
}

}
