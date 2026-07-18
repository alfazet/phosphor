#include "logger.hpp"

inline const char *logger::level_to_string(logger::Level lvl) {
    switch (lvl) {
    case logger::Level::Debug:
        return "DEBUG";
    case logger::Level::Info:
        return "INFO";
    case logger::Level::Warning:
        return "WARN";
    case logger::Level::Error:
        return "ERROR";
    case logger::Level::Fatal:
        return "FATAL";
    }
    // can't use unreachable because of recursion
    std::fprintf(stderr, "level_to_string: unknown log level\n");
    LOG_TRAP();
    return "UNKNOWN";
}

inline const char *level_to_color(logger::Level lvl) {
    switch (lvl) {
    case logger::Level::Debug:
        return "\033[36m";
    case logger::Level::Info:
        return "\033[32m";
    case logger::Level::Warning:
        return "\033[33m";
    case logger::Level::Error:
        return "\033[31m";
    case logger::Level::Fatal:
        return "\033[35m";
    }
    std::fprintf(stderr, "level_to_string: unknown log level\n");
    LOG_TRAP();
    return "UNKNOWN";
}

constexpr const char *reset_color = "\033[0m";

inline std::mutex &console_mutex() {
    static std::mutex m;
    return m;
}
