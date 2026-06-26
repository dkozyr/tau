#pragma once

#include <tau/sdp/Mid.h>
#include <tau/sdp/Ice.h>
#include <tau/sdp/Dtls.h>
#include <tau/sdp/Media.h>
#include <memory>
#include <initializer_list>

namespace tau::sdp {

using CName = etl::string<48>;

struct Sdp {
    CName cname = {};
    BundleMids bundle_mids = {};
    std::optional<Ice> ice = std::nullopt;
    std::optional<Dtls> dtls = std::nullopt;
    Medias medias = {};
};
using SdpPtr = std::unique_ptr<Sdp>;

SdpPtr ParseSdp(etl::string_view sdp_str);
etl::istring& WriteSdp(etl::istring& output, const Sdp& sdp);

inline BundleMids MakeBundleMids(std::initializer_list<etl::string_view> list) {
    BundleMids bundle_mids;
    for(auto&& mid : list) {
        if(bundle_mids.full()) {
            break;
        }
        bundle_mids.push_back(Mid{mid});
    }
    return bundle_mids;
}

}
