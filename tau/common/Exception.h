#pragma once

#include <tau/common/Log.h>
#ifdef ESP_PLATFORM
    #include <cstdlib>
#else
    #include <stdexcept>
    #include <etl/string_stream.h>
#endif

namespace tau {

#ifdef ESP_PLATFORM

#define TAU_EXCEPTION(exception, message) {  \
        TAU_LOG_FATAL(message);              \
        std::abort();                        \
    }

#else

#define TAU_EXCEPTION(exception, message) {               \
        tau::g_log_text.clear();                          \
        etl::string_stream exception_ss(tau::g_log_text); \
        exception_ss << (DETAIL_LOG_CONTEXT) << message;  \
        throw exception(exception_ss.str().c_str());      \
    }

#endif

}
