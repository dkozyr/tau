#pragma once

#include "tau/sdp/Ice.h"
#include "tau/sdp/Dtls.h"
#include "tau/sdp/Media.h"
#include <memory>
#include <initializer_list>

namespace tau::sdp {

using CName = etl::string<16>;
using BundleMid = etl::string<8>;
inline constexpr size_t kMaxBundleMids = 4;

struct Sdp {
    CName cname = {};
    etl::vector<BundleMid, kMaxBundleMids> bundle_mids = {};
    std::optional<Ice> ice = std::nullopt;
    std::optional<Dtls> dtls = std::nullopt;
    Medias medias = {};
};
using SdpPtr = std::unique_ptr<Sdp>;

SdpPtr ParseSdp(etl::string_view sdp_str);
etl::istring& WriteSdp(etl::istring& output, const Sdp& sdp);

inline etl::vector<BundleMid, kMaxBundleMids> MakeBundleMids(std::initializer_list<etl::string_view> list) {
    etl::vector<BundleMid, kMaxBundleMids> bundle_mids;
    for(auto&& mid : list) {
        bundle_mids.push_back(BundleMid{mid});
    }
    return bundle_mids;
}

}
