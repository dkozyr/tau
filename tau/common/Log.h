#pragma once

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <array>

inline void InitLogging(const std::string& file_name = "log", bool console = false) {
    namespace logging = boost::log;
    namespace keywords = boost::log::keywords;
    namespace attrs = boost::log::attributes;

    logging::register_simple_formatter_factory<logging::trivial::severity_level, char>("Severity");
    logging::add_file_log(
        keywords::file_name = file_name + "_%N.log",
        keywords::rotation_size = 10 * 1024 * 1024,
        keywords::auto_flush = true,
        keywords::format = "[%TimeStamp%] [%ThreadID%] [%Severity%] %Message%"
    );
    logging::core::get()->set_filter(
        logging::trivial::severity >= logging::trivial::info
    );
    logging::add_common_attributes();
    if(console) {
        logging::add_console_log(std::cout, boost::log::keywords::format = "[%TimeStamp%] [%ThreadID%] [%Severity%] %Message%");
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
#define DETAIL_LOG_CONTEXT << "[" << DETAIL_LOG_FILENAME << ":" << __LINE__ << "] [" << __FUNCTION__ << "] "

#define LOG_TRACE BOOST_LOG_TRIVIAL(trace) DETAIL_LOG_CONTEXT
#define LOG_DEBUG BOOST_LOG_TRIVIAL(debug) DETAIL_LOG_CONTEXT
#define LOG_INFO BOOST_LOG_TRIVIAL(info) DETAIL_LOG_CONTEXT
#define LOG_WARNING BOOST_LOG_TRIVIAL(warning) DETAIL_LOG_CONTEXT
#define LOG_ERROR BOOST_LOG_TRIVIAL(error) DETAIL_LOG_CONTEXT
#define LOG_FATAL BOOST_LOG_TRIVIAL(fatal) DETAIL_LOG_CONTEXT
