#pragma once

#include <tau/common/LogSeverity.h>
#include "esp_log.h"
#include <etl/string.h>
#include <etl/string_stream.h>

namespace tau {

#define STRINGIFY(x) #x
#define TO_STRING(x) STRINGIFY(x)

#define DETAIL_LOG_CONTEXT __FILE_NAME__ ":" TO_STRING(__LINE__)

extern etl::string<2048> g_log_text;

#define LOG_TRIVIAL(log, message)                                               \
    do {                                                                        \
        tau::g_log_text.clear();                                                \
        etl::string_stream ss_log(tau::g_log_text);                             \
        ss_log << message;                                                      \
        log(DETAIL_LOG_CONTEXT, "[%s] %s", __FUNCTION__, ss_log.str().c_str()); \
    } while(0)

#define DETAIL_TAU_LOG(severity, log, message)                                  \
    if constexpr(severity >= tau::kSeverityDefault) {                           \
        LOG_TRIVIAL(log, message);                                              \
    }

#define DETAIL_TAU_LOG_THR(threshold, severity, log, message)                   \
    if constexpr(severity >= tau::kSeverityDefault) {                           \
        static size_t value = threshold - 1;                                    \
        if(++value % threshold == 0) {                                          \
            LOG_TRIVIAL(log, message);                                          \
        }                                                                       \
    }

#define TAU_LOG_FATAL(message)   DETAIL_TAU_LOG(tau::Severity::kFatal,   ESP_LOGE, message)
#define TAU_LOG_ERROR(message)   DETAIL_TAU_LOG(tau::Severity::kError,   ESP_LOGE, message)
#define TAU_LOG_WARNING(message) DETAIL_TAU_LOG(tau::Severity::kWarning, ESP_LOGW, message)
#define TAU_LOG_INFO(message)    DETAIL_TAU_LOG(tau::Severity::kInfo,    ESP_LOGI, message)
#define TAU_LOG_DEBUG(message)   DETAIL_TAU_LOG(tau::Severity::kDebug,   ESP_LOGD, message)
#define TAU_LOG_TRACE(message)   DETAIL_TAU_LOG(tau::Severity::kTrace,   ESP_LOGV, message)

#define TAU_LOG_ERROR_THR(threshold, message)   DETAIL_TAU_LOG_THR(threshold, tau::Severity::kError,   ESP_LOGE, message)
#define TAU_LOG_WARNING_THR(threshold, message) DETAIL_TAU_LOG_THR(threshold, tau::Severity::kWarning, ESP_LOGW, message)
#define TAU_LOG_INFO_THR(threshold, message)    DETAIL_TAU_LOG_THR(threshold, tau::Severity::kInfo,    ESP_LOGI, message)
#define TAU_LOG_DEBUG_THR(threshold, message)   DETAIL_TAU_LOG_THR(threshold, tau::Severity::kDebug,   ESP_LOGD, message)
#define TAU_LOG_TRACE_THR(threshold, message)   DETAIL_TAU_LOG_THR(threshold, tau::Severity::kTrace,   ESP_LOGV, message)

}
