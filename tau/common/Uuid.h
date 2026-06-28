#pragma once

#include <etl/string.h>
#include <etl/string_view.h>

namespace tau {

constexpr auto kUuidSize = 36;
using Uuid = etl::string<kUuidSize>;

Uuid GenerateUuid();
bool IsUuidTrivialCheck(etl::string_view uuid);

}
