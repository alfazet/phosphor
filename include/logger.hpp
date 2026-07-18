#ifndef PHOSPHOR_LOGGER_HPP
#define PHOSPHOR_LOGGER_HP

#include <atomic>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <format>
#include <memory>
#include <mutex>
#include <source_location>
#include <string>
#include <string_view>
#include <vector>

#include "common.hpp"

namespace logger {

enum class Level { Debug, Info, Warning, Error, Fatal };

const char *level_to_string(Level level);

inline std::mutex &console_mutex() {
    static std::mutex m;
    return m;
}

class Sink {
  public:
    virtual ~Sink() = default;
    virtual void write(std::string_view formatted_record) = 0;
    virtual void flush() = 0;
};

class ConsoleSink : public Sink {
  public:
    explicit ConsoleSink(bool colored = true) : colored_(colored) {}

    void write(std::string_view msg) override {
        std::lock_guard<std::mutex> lock(console_mutex());
        // TODO: colored
        std::fwrite(msg.data(), 1, msg.size(), stdout);
    }

    void flush() override {
        std::lock_guard<std::mutex> lock(console_mutex());
        std::fflush(stdout);
    }

  private:
    bool colored_;
};

class FileSink : public Sink {
  public:
    explicit FileSink(std::string_view path, bool append = true);
    ~FileSink() override;
    void write(std::string_view msg) override;
    void flush() override;
};

class MultiSink : public Sink {
  public:
    void add(std::unique_ptr<Sink> sink);
    void write(std::string_view msg) override;
    void flush() override;

  private:
    std::vector<std::unique_ptr<Sink>> sinks_;
    std::mutex mutex_;
};

class Logger {
  public:
    static Logger &instance() {
        static Logger logger;
        return logger;
    }

    void set_level(Level min_level) { min_level_ = min_level; }

    void set_sink(std::unique_ptr<Sink> sink) {
        std::lock_guard<std::mutex> lock(mutex_);
        sink_ = std::move(sink);
    }

    void add_sink(std::unique_ptr<Sink> sink) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!sink_) {
            sink_ = std::move(sink);
            return;
        }

        if (auto *multi = dynamic_cast<MultiSink *>(sink.get())) {
            multi->add(std::move(sink));
            return;
        }

        auto multi = std::make_unique<MultiSink>();
        multi->add(std::move(sink_));
        multi->add(std::move(sink));
        sink_ = std::move(multi);
    }

    bool enabled(Level level) const { return static_cast<int>(level) >= static_cast<int>(min_level_); };

    template <typename... Args>
    void log(Level level, std::source_location loc, std::format_string<Args...> fmt, Args &&...args) {
        if (!enabled(level))
            return;

        // std::forward combined with a forwarding reference (Args&&) restores the original value category
        // of the argument. ex. passing a temporary std::string("abc") lets std::format move it instead
        // of copying, while passing an lvalue string is forwarded as an lvalue and not moved from.
        std::string message = std::format(fmt, std::forward<Args>(args)...);
        std::string filename = std::filesystem::path(loc.file_name()).filename().string();
        std::string record = std::format("[{:5} {}:{}] {}\n", level_to_string(level), filename, loc.line(), message);

        std::lock_guard<std::mutex> lock(mutex_);
        sink_->write(record);
    }

    template <typename... Args>
    void assert_fail(std::string_view condition, std::source_location loc, std::format_string<Args...> fmt,
                     Args &&...args) {
        std::string message = std::format(fmt, std::forward<Args>(args)...);
        log(Level::Fatal, loc, "assertion '{}' failed: {}", condition, message);
    }

  private:
    Level min_level_ = Level::Info;
    std::unique_ptr<Sink> sink_;
    std::mutex mutex_;
};

class ProgressScope {
  public:
    ProgressScope(std::string_view name, usize total,
                  std::chrono::milliseconds min_update_interval = std::chrono::milliseconds{250});
    ~ProgressScope();

    void update(usize current);
    void finish();

  private:
    std::string name_;
    usize total_;
    std::chrono::steady_clock::time_point start_;
    std::chrono::milliseconds min_interval_;
    std::atomic<usize> current_{0};
    std::chrono::steady_clock::time_point last_render_;
    std::mutex render_mutex_;
};

class TimerScope {
  public:
    explicit TimerScope(std::string_view name);
    ~TimerScope();

  private:
    std::string name_;
    std::chrono::steady_clock::time_point start_;
};
} // namespace logger

#if defined(__GNUC__) || defined(__clang__)
#define LOG_TRAP() __builtin_trap()
#else
#define LOG_TRAP() std::abort()
#endif

#ifndef ACTIVE_LOG_LEVEL
#ifdef NDEBUG
#define ACTIVE_LOG_LEVEL logger::Level::Info
#else
#define ACTIVE_LOG_LEVEL logger::Level::Debug
#endif
#endif

#define LOG_DETAIL(level, fmt, ...)                                                                                    \
    do {                                                                                                               \
        if constexpr (static_cast<int>(level) >= static_cast<int>(ACTIVE_LOG_LEVEL)) {                                 \
            logger::Logger::instance().log(level, std::source_location::current(), fmt __VA_OPT__(, ) __VA_ARGS__);    \
        }                                                                                                              \
    } while (0)

#define LOG_DEBUG(fmt, ...) LOG_DETAIL(logger::Level::Debug, fmt __VA_OPT__(, ) __VA_ARGS__)
#define LOG_INFO(fmt, ...) LOG_DETAIL(logger::Level::Info, fmt __VA_OPT__(, ) __VA_ARGS__)
#define LOG_WARN(fmt, ...) LOG_DETAIL(logger::Level::Warning, fmt __VA_OPT__(, ) __VA_ARGS__)
#define LOG_ERROR(fmt, ...) LOG_DETAIL(logger::Level::Error, fmt __VA_OPT__(, ) __VA_ARGS__)
#define LOG_FATAL(fmt, ...) LOG_DETAIL(logger::Level::Fatal, fmt __VA_OPT__(, ) __VA_ARGS__)

#define DBG(x)                                                                                                         \
    do {                                                                                                               \
        if constexpr (static_cast<int>(logger::Level::Debug) >= static_cast<int>(ACTIVE_LOG_LEVEL)) {                  \
            logger::Logger::instance().log(logger::Level::Debug, std::source_location::current(), "{} = {}", #x, (x)); \
        }                                                                                                              \
    } while (0)

#define ASSERT(cond, fmt, ...)                                                                                         \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            logger::Logger::instance().assert_fail(#cond, std::source_location::current(),                             \
                                                   fmt __VA_OPT__(, ) __VA_ARGS__);                                    \
            LOG_TRAP();                                                                                                \
        }                                                                                                              \
    } while (0)

#define UNREACHABLE(...)                                                                                               \
    do {                                                                                                               \
        logger::Logger::instance().log(logger::Level::Fatal, std::source_location::current(),                          \
                                       "unreachable code reached" __VA_OPT__(": ") __VA_ARGS__);                       \
        LOG_TRAP();                                                                                                    \
    } while (0)

#define UNIMPLEMENTED(...)                                                                                             \
    do {                                                                                                               \
        logger::Logger::instance().log(logger::Level::Fatal, std::source_location::current(),                          \
                                       "unimplemented code reached" __VA_OPT__(": ") __VA_ARGS__);                     \
        LOG_TRAP();                                                                                                    \
    } while (0)

#define LOG_PROGRESS(name, total) logger::ProgressScope _progress(name, total)
#define LOG_TIMER(name) logger::TimerScope _timer(name)

#endif // PHOSPHOR_LOGGER_HPP
