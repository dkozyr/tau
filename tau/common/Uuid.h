#pragma once

#include <etl/string.h>
#include <etl/string_view.h>

namespace tau {

using Uuid = etl::string<36>;

Uuid GenerateUuid();
bool IsUuidTrivialCheck(etl::string_view uuid);

}
