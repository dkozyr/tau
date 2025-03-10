#pragma once

#include "tests/TestClock.h"
#include "tau/memory/SystemAllocator.h"
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

inline Random g_random;
