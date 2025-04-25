#pragma once

#include <string>
#include <string_view>

namespace tau {

std::string GenerateUuid();
bool IsUuidTrivialCheck(std::string_view uuid);

}
