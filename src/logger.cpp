#include "logger.hpp"
#include "common.hpp"

const char *logger::level_to_string(logger::Level lvl) {
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

const char *logger::level_to_color(logger::Level lvl) {
    switch (lvl) {
    case logger::Level::Debug:
        return "\033[36m";
    case logger::Level::Info:
        return "\033[34m";
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

std::optional<logger::Level> logger::parse_level_from_record(std::string_view record) {
    switch (record[1]) {
    case 'D':
        return logger::Level::Debug;
    case 'I':
        return logger::Level::Info;
    case 'W':
        return logger::Level::Warning;
    case 'E':
        return logger::Level::Error;
    case 'F':
        return logger::Level::Fatal;
    }

    return std::nullopt;
}
