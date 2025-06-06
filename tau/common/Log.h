#pragma once

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <array>

namespace tau {

//TODO: make it configurable in compile/runtime
inline constexpr auto kSeverityDefault = 1;
inline const std::array<boost::log::trivial::severity_level, 6> kSeverity = {
    boost::log::trivial::trace,
    boost::log::trivial::debug,
    boost::log::trivial::info,
    boost::log::trivial::warning,
    boost::log::trivial::error,
    boost::log::trivial::fatal,
};

inline void InitLogging(const std::string& file_name = "log", bool console = false, size_t severity = kSeverityDefault) {
    boost::log::register_simple_formatter_factory<boost::log::trivial::severity_level, char>("Severity");
    boost::log::add_file_log(
        boost::log::keywords::file_name = file_name + "_%N.log",
        boost::log::keywords::rotation_size = 10 * 1024 * 1024,
        boost::log::keywords::auto_flush = true,
        boost::log::keywords::format = "[%TimeStamp%] [%ThreadID%] [%Severity%] %Message%"
    );
    boost::log::core::get()->set_filter(
        boost::log::trivial::severity >= kSeverity[severity]
    );
    boost::log::add_common_attributes();
    if(console) {
        boost::log::add_console_log(std::cout,
            boost::log::keywords::format = "[%TimeStamp%] [%ThreadID%] [%Severity%] %Message%");
    }
}

inline constexpr std::string_view TruncateFileName(std::string_view path) {
    const std::array<std::string_view, 2> dirs{"/tau/", "/../"};
    for(auto& dir: dirs) {
        auto p = path.rfind(dir);
        if(p != std::string_view::npos) {
            path = path.substr(p + dir.size());
        }
    }
    return path;
}

#define DETAIL_LOG_FILENAME TruncateFileName("/" __FILE__)
#define DETAIL_LOG_CONTEXT "[" << DETAIL_LOG_FILENAME << ":" << __LINE__ << "] [" << __FUNCTION__ << "] "

#define DETAIL_TAU_LOG(severity, message)                             \
    if constexpr(boost::log::trivial::severity >= kSeverityDefault) { \
        BOOST_LOG_TRIVIAL(severity) << DETAIL_LOG_CONTEXT << message; \
    }

#define DETAIL_TAU_LOG_THR(threshold, severity, message)                  \
    if constexpr(boost::log::trivial::severity >= kSeverityDefault) {     \
        static size_t value = threshold - 1;                              \
        if(++value % threshold == 0) {                                    \
            BOOST_LOG_TRIVIAL(severity) << DETAIL_LOG_CONTEXT << message; \
        }                                                                 \
    }

#define TAU_LOG_FATAL(message)   DETAIL_TAU_LOG(fatal, message)
#define TAU_LOG_ERROR(message)   DETAIL_TAU_LOG(error, message)
#define TAU_LOG_WARNING(message) DETAIL_TAU_LOG(warning, message)
#define TAU_LOG_INFO(message)    DETAIL_TAU_LOG(info,  message)
#define TAU_LOG_DEBUG(message)   DETAIL_TAU_LOG(debug, message)
#define TAU_LOG_TRACE(message)   DETAIL_TAU_LOG(trace, message)

#define TAU_LOG_ERROR_THR(threshold, message)   DETAIL_TAU_LOG_THR(threshold, error, message)
#define TAU_LOG_WARNING_THR(threshold, message) DETAIL_TAU_LOG_THR(threshold, warning, message)
#define TAU_LOG_INFO_THR(threshold, message)    DETAIL_TAU_LOG_THR(threshold, info,  message)
#define TAU_LOG_DEBUG_THR(threshold, message)   DETAIL_TAU_LOG_THR(threshold, debug, message)
#define TAU_LOG_TRACE_THR(threshold, message)   DETAIL_TAU_LOG_THR(threshold, trace, message)

}
