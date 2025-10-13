#pragma once

#include <iostream>
#include <sstream>
#include <string>

namespace ggg {
namespace utils {

/**
 * @brief Logging levels for the ggg logging system
 */
enum class LogLevel : int {
    NONE = 0,  ///< Complete disable - no logging
    ERROR = 1, ///< Error messages only
    WARN = 2,  ///< Warning and error messages
    INFO = 3,  ///< Information, warning and error messages (default)
    DEBUG = 4, ///< Debug, information, warning and error messages
    TRACE = 5  ///< All messages including trace
};

#ifdef ENABLE_LOGGING

// Runtime logging level - can be set via command line
inline LogLevel g_runtime_log_level = LogLevel::WARN;

/**
 * @brief Set the runtime logging level
 */
inline void set_log_level(LogLevel level) {
    g_runtime_log_level = level;
}

/**
 * @brief Get the current runtime logging level
 */
inline LogLevel get_log_level() {
    return g_runtime_log_level;
}

/**
 * @brief Convert verbosity count to log level
 */
inline LogLevel verbosity_to_log_level(int verbosity) {
    switch (verbosity) {
    case 0:
        return LogLevel::WARN; // Default level
    case 1:
        return LogLevel::INFO; // -v
    case 2:
        return LogLevel::DEBUG; // -vv
    default:
        return LogLevel::TRACE; // -vvv or more
    }
}

/**
 * @brief Internal logging function
 */
template <typename... Args>
inline void log_message(LogLevel level, const std::string &prefix, Args &&...args) {
    if (static_cast<int>(level) <= static_cast<int>(g_runtime_log_level)) {
        std::ostringstream oss;
        oss << prefix << ": ";
        (oss << ... << args);
        std::cerr << oss.str() << std::endl;
    }
}

// Compile-time check if logging level is enabled
#ifndef LOG_LEVEL
#define LOG_LEVEL 2 // Default to WARN level
#endif

#if LOG_LEVEL >= 1
#define LGG_ERROR(...) ::ggg::utils::log_message(::ggg::utils::LogLevel::ERROR, "ERROR", __VA_ARGS__)
#else
#define LGG_ERROR(...) ((void)0)
#endif

#if LOG_LEVEL >= 2
#define LGG_WARN(...) ::ggg::utils::log_message(::ggg::utils::LogLevel::WARN, "WARN", __VA_ARGS__)
#else
#define LGG_WARN(...) ((void)0)
#endif

#if LOG_LEVEL >= 3
#define LGG_INFO(...) ::ggg::utils::log_message(::ggg::utils::LogLevel::INFO, "INFO", __VA_ARGS__)
#else
#define LGG_INFO(...) ((void)0)
#endif

#if LOG_LEVEL >= 4
#define LGG_DEBUG(...) ::ggg::utils::log_message(::ggg::utils::LogLevel::DEBUG, "DEBUG", __VA_ARGS__)
#else
#define LGG_DEBUG(...) ((void)0)
#endif

#if LOG_LEVEL >= 5
#define LGG_TRACE(...) ::ggg::utils::log_message(::ggg::utils::LogLevel::TRACE, "TRACE", __VA_ARGS__)
#else
#define LGG_TRACE(...) ((void)0)
#endif

#else // ENABLE_LOGGING not defined

// When logging is disabled, all macros compile to no-ops
#define LGG_ERROR(...) ((void)0)
#define LGG_WARN(...) ((void)0)
#define LGG_INFO(...) ((void)0)
#define LGG_DEBUG(...) ((void)0)
#define LGG_TRACE(...) ((void)0)

// Dummy functions for when logging is disabled
inline void set_log_level(LogLevel) {}
inline LogLevel get_log_level() { return LogLevel::NONE; }
inline LogLevel verbosity_to_log_level(int) { return LogLevel::NONE; }

#endif // ENABLE_LOGGING

} // namespace utils
} // namespace ggg
