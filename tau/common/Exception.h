#pragma once

#include "tau/common/Log.h"
#include <exception>
#include <sstream>

namespace tau {

#define TAU_EXCEPTION(exception, message) {  \
        std::stringstream ss;                \
        ss << DETAIL_LOG_CONTEXT << message; \
        throw exception(ss.str());           \
    }

}
