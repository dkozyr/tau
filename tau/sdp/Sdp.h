#pragma once

#include "tau/sdp/Ice.h"
#include "tau/sdp/Dtls.h"
#include "tau/sdp/Media.h"
#include <memory>
#include <initializer_list>

namespace tau::sdp {

struct Sdp {
    etl::string<64> cname = {};
    etl::vector<etl::string<32>, 4> bundle_mids = {};
    std::optional<Ice> ice = std::nullopt;
    std::optional<Dtls> dtls = std::nullopt;
    Medias medias = {};
};

using SdpPtr = std::unique_ptr<Sdp>;

SdpPtr ParseSdp(etl::string_view sdp_str);
etl::istring& WriteSdp(etl::istring& output, const Sdp& sdp);

inline etl::vector<etl::string<32>, 4> MakeBundleMids(std::initializer_list<etl::string_view> list) {
    etl::vector<etl::string<32>, 4> bundle_mids;
    for(auto&& mid : list) {
        bundle_mids.push_back(etl::string<32>{mid});
    }
    return bundle_mids;
}

}
