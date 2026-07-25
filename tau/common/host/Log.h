#pragma once

#include <tau/common/LogSeverity.h>
#include <etl/string_stream.h>
#include <etl/array.h>
#include <stdio.h>
#include <sstream>
#include <thread>
#include <ctime>
#include <chrono>

namespace tau {

inline static constexpr etl::array<const char*, 6> g_severity_array = {
    "trace",
    "debug",
    "info",
    "warning",
    "error",
    "fatal",
};

#define STRINGIFY(x) #x
#define TO_STRING(x) STRINGIFY(x)

#define DETAIL_LOG_CONTEXT __FILE_NAME__ ":" TO_STRING(__LINE__)

inline static etl::string<2048> g_log_text;

#define LOG_TRIVIAL(severity, message)                                   \
    do {                                                                 \
        tau::g_log_text.clear();                                         \
        etl::string_stream ss_log(tau::g_log_text);                      \
        ss_log << message;                                               \
        std::stringstream ss;                                            \
        auto now = std::chrono::steady_clock{}.now().time_since_epoch(); \
        auto ms = std::chrono::duration<float>(now).count();             \
        auto id = std::this_thread::get_id();                            \
        ss << std::fixed << std::setprecision(3) << "[" << ms << "] ";   \
        ss << "[" << id << "]" ;                                         \
        printf("%s [%s] [%s] [%s] %s\n", ss.str().c_str(), tau::g_severity_array[severity], DETAIL_LOG_CONTEXT, __FUNCTION__, ss_log.str().c_str()); \
    } while(0);

#define DETAIL_TAU_LOG(severity, message)                              \
    if constexpr(severity >= tau::kSeverityDefault) {                  \
        LOG_TRIVIAL(severity, message);                                \
    }

#define DETAIL_TAU_LOG_THR(threshold, severity, message)               \
    if constexpr(severity >= tau::kSeverityDefault) {                  \
        static size_t value = threshold - 1;                           \
        if(++value % threshold == 0) {                                 \
            LOG_TRIVIAL(severity, message);                            \
        }                                                              \
    }

#define TAU_LOG_FATAL(message)   DETAIL_TAU_LOG(Severity::kFatal, message)
#define TAU_LOG_ERROR(message)   DETAIL_TAU_LOG(Severity::kError, message)
#define TAU_LOG_WARNING(message) DETAIL_TAU_LOG(Severity::kWarning, message)
#define TAU_LOG_INFO(message)    DETAIL_TAU_LOG(Severity::kInfo,  message)
#define TAU_LOG_DEBUG(message)   DETAIL_TAU_LOG(Severity::kDebug, message)
#define TAU_LOG_TRACE(message)   DETAIL_TAU_LOG(Severity::kTrace, message)

#define TAU_LOG_ERROR_THR(threshold, message)   DETAIL_TAU_LOG_THR(threshold, Severity::kError, message)
#define TAU_LOG_WARNING_THR(threshold, message) DETAIL_TAU_LOG_THR(threshold, Severity::kWarning, message)
#define TAU_LOG_INFO_THR(threshold, message)    DETAIL_TAU_LOG_THR(threshold, Severity::kInfo, message)
#define TAU_LOG_DEBUG_THR(threshold, message)   DETAIL_TAU_LOG_THR(threshold, Severity::kDebug, message)
#define TAU_LOG_TRACE_THR(threshold, message)   DETAIL_TAU_LOG_THR(threshold, Severity::kTrace, message)

}
