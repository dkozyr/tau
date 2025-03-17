#pragma once

#include "tests/lib/TestClock.h"
#include "tau/memory/SystemAllocator.h"
#include "tau/memory/PoolAllocator.h"
#include "tau/common/String.h"
#include "tau/common/Container.h"
#include "tau/common/Random.h"
#include "tau/common/Log.h"

#include <gtest/gtest.h>

#include <vector>
#include <optional>
#include <functional>
#include <algorithm>
#include <string_view>
#include <cstring>

namespace tau {

using namespace std::chrono_literals;

inline Random g_random;
inline PoolAllocator g_udp_allocator(kUdpMtuSize);

}
