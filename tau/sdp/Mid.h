#pragma once

#include <etl/string.h>
#include <etl/vector.h>

namespace tau::sdp {

inline constexpr size_t kMaxBundleMids = 4;

using Mid = etl::string<8>;
using BundleMids = etl::vector<Mid, kMaxBundleMids>;

}
