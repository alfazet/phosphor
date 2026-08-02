#ifndef PHOSPHOR_LOGGER_HPP
#define PHOSPHOR_LOGGER_HPP

#include <atomic>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <mutex>
#include <source_location>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "common.hpp"

namespace logger {

enum class Level { Debug, Info, Warning, Error, Fatal };

const char *level_to_string(Level level);
const char *level_to_color(Level lvl);
std::optional<Level> parse_level_from_record(std::string_view record);
std::string format_duration(std::chrono::steady_clock::duration d);

constexpr const char *reset_color = "\033[0m";

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

        std::optional<Level> level;
        usize close = msg.find(']', 1);
        if (!colored_ || !(level = parse_level_from_record(msg)) || (close == std::string_view::npos)) {
            std::fwrite(msg.data(), 1, msg.size(), stdout);
            return;
        }

        std::fwrite(msg.data(), 1, 0, stdout);

        std::fwrite(level_to_color(*level), 1, std::strlen(level_to_color(*level)), stdout);
        std::fwrite(msg.data(), 1, close + 1, stdout);
        std::fwrite(reset_color, 1, std::strlen(reset_color), stdout);

        std::fwrite(msg.data() + close + 1, 1, msg.size() - close, stdout);
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
    explicit FileSink(std::string_view path, bool append = true) {
        auto mode = append ? std::ios::app : std::ios::trunc;
        file_.open(std::string(path), std::ios::out | mode);
    }

    void write(std::string_view msg) override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (file_.is_open()) {
            file_.write(msg.data(), static_cast<std::streamsize>(msg.size()));
        }
    }

    void flush() override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (file_.is_open()) {
            file_.flush();
        }
    }

  private:
    std::ofstream file_;
    std::mutex mutex_;
};

class MultiSink : public Sink {
  public:
    void add(std::unique_ptr<Sink> sink) {
        std::lock_guard<std::mutex> lock(mutex_);
        sinks_.push_back(std::move(sink));
    }

    void write(std::string_view msg) override {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto &sink : sinks_) {
            sink->write(msg);
        }
    }

    void flush() override {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto &sink : sinks_) {
            sink->flush();
        }
    }

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

    bool enabled(Level level) const { return static_cast<i32>(level) >= static_cast<i32>(min_level_); };

    template <typename... Args>
    void log(Level level, std::source_location loc, std::format_string<Args...> fmt, Args &&...args) {
        if (!enabled(level))
            return;

        // std::forward combined with a forwarding reference (Args&&) restores the original value category
        // of the argument. ex. passing a temporary std::string("abc") lets std::format move it instead
        // of copying, while passing an lvalue string is forwarded as an lvalue and not moved from.
        std::string message = std::format(fmt, std::forward<Args>(args)...);
        std::string filename = std::filesystem::path(loc.file_name()).filename().string();
        std::string record = std::format("[{} {}:{}] {}\n", level_to_string(level), filename, loc.line(), message);

        std::lock_guard<std::mutex> lock(mutex_);
        sink_->write(record);
    }

    template <typename... Args>
    void assert_fail(std::string_view condition, std::source_location loc, std::format_string<Args...> fmt,
                     Args &&...args) {
        std::string message = std::format(fmt, std::forward<Args>(args)...);
        log(Level::Fatal, loc, "assertion '{}' failed: {}", condition, message);
    }

    void flush() { sink_->flush(); }

  private:
    Level min_level_ = Level::Info;
    std::unique_ptr<Sink> sink_;
    std::mutex mutex_;
};
} // namespace logger

#if defined(__GNUC__) || defined(__clang__)
#define LOG_TRAP() __builtin_trap()
#else
#define LOG_TRAP() std::abort()
#endif

#ifndef ACTIVE_LOG_LEVEL
#define ACTIVE_LOG_LEVEL logger::Level::Debug
#endif

#define LOG_DETAIL(level, fmt, ...)                                                                                    \
    do {                                                                                                               \
        if constexpr (static_cast<i32>(level) >= static_cast<i32>(ACTIVE_LOG_LEVEL)) {                                 \
            logger::Logger::instance().log(level, std::source_location::current(), fmt __VA_OPT__(, ) __VA_ARGS__);    \
        }                                                                                                              \
    } while (0)

#define LOG_DEBUG(fmt, ...) LOG_DETAIL(logger::Level::Debug, fmt __VA_OPT__(, ) __VA_ARGS__)
#define LOG_INFO(fmt, ...) LOG_DETAIL(logger::Level::Info, fmt __VA_OPT__(, ) __VA_ARGS__)
#define LOG_WARN(fmt, ...) LOG_DETAIL(logger::Level::Warning, fmt __VA_OPT__(, ) __VA_ARGS__)
#define LOG_ERROR(fmt, ...) LOG_DETAIL(logger::Level::Error, fmt __VA_OPT__(, ) __VA_ARGS__)
#define LOG_FATAL(fmt, ...)                                                                                            \
    do {                                                                                                               \
        logger::Logger::instance().log(logger::Level::Fatal, std::source_location::current(),                          \
                                       fmt __VA_OPT__(, ) __VA_ARGS__);                                                \
        LOG_TRAP();                                                                                                    \
    } while (0)

#define DBG(x)                                                                                                         \
    do {                                                                                                               \
        if constexpr (static_cast<i32>(logger::Level::Debug) >= static_cast<i32>(ACTIVE_LOG_LEVEL)) {                  \
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

#define LOGGER_FLUSH() logger::Logger::instance().flush();

class ProgressScope {
  public:
    ProgressScope(std::string_view name, usize total,
                  std::chrono::milliseconds min_update_interval = std::chrono::milliseconds{250})
        : name_(name), total_(total), start_(std::chrono::steady_clock::now()), min_interval_(min_update_interval),
          last_render_(start_) {
        render();
    }

    void update(usize current) {
        current_.store(current, std::memory_order_relaxed);
        render();
    }

    void increase(usize amount) {
        current_.fetch_add(amount, std::memory_order_relaxed);
        render();
    }

  private:
    void render() {
        std::lock_guard<std::mutex> lock(render_mutex_);
        const usize current = current_.load(std::memory_order_relaxed);
        auto now = std::chrono::steady_clock::now();

        bool final = current >= total_;
        if (!first_render_ && !final && now - last_render_ < min_interval_)
            return;

        first_render_ = false;
        last_render_ = now;

        auto elapsed = now - start_;
        f64 fraction = total_ > 0 ? static_cast<f64>(current) / static_cast<f64>(total_) : 0.0;
        fraction = std::clamp(fraction, 0.0, 1.0);

        i32 percent = static_cast<i32>(fraction * 100.0);
        constexpr i32 width = 40;
        i32 filled = static_cast<i32>(fraction * static_cast<f64>(width));
        filled = std::clamp(filled, 0, width);

        std::string bar;
        bar.reserve(width + 2);
        bar += '[';
        bar.append(filled, '=');
        bar.append(width - filled, '-');
        bar += ']';

        std::string eta_str;
        if (current == 0) {
            eta_str = ", eta ?";
        } else if (current < total_) {
            auto per_item = elapsed / current;
            auto remaining = per_item * (total_ - current);
            eta_str = std::format(", eta {}", logger::format_duration(remaining));
        }

        std::string line =
            std::format("\r\033[K{} {} {:3}% ({}{})", name_, bar, percent, logger::format_duration(elapsed), eta_str);

        {
            std::unique_lock<std::mutex> lock(logger::console_mutex());
            std::fwrite(line.data(), 1, line.size(), stdout);
            if (final) {
                std::fputc('\n', stdout);
                lock.unlock();
                print_outro(now);
            }
            std::fflush(stdout);
        }
    }

    void print_outro(std::chrono::steady_clock::time_point now) {
        LOG_INFO("finished {}, took {}", name_, logger::format_duration(now - start_));
    }

    std::string name_;
    usize total_;
    bool first_render_;
    std::chrono::steady_clock::time_point start_;
    std::chrono::milliseconds min_interval_;
    std::atomic<usize> current_{0};
    std::chrono::steady_clock::time_point last_render_;
    std::mutex render_mutex_;
};

class TimerScope {
  public:
    explicit TimerScope(std::string_view name, bool real_time = false,
                        std::chrono::milliseconds min_update_interval = std::chrono::milliseconds{250})
        : name_(name), min_interval_(min_update_interval), real_time_(real_time) {
        if (real_time_) {
            render();
            thread_ = std::thread([this]() {
                while (!stopped_) {
                    std::this_thread::sleep_for(min_interval_);
                    render();
                }
            });
        } else {
            print_intro();
        }
    }

    ~TimerScope() { stop(); }

    void stop() {
        auto now = std::chrono::steady_clock::now();
        if (stopped_.exchange(true))
            return;
        if (real_time_) {
            thread_.join();
            render(true, now);
        }
        print_outro(now);
    }

  private:
    void render(bool final = false, std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now()) {
        auto elapsed = now - start_;
        std::string line = std::format("\r\033[K{}: {}", name_, logger::format_duration(elapsed));

        std::lock_guard<std::mutex> lock(logger::console_mutex());
        std::fwrite(line.data(), 1, line.size(), stdout);
        if (final)
            std::fputc('\n', stdout);
        std::fflush(stdout);
    }

    void print_intro() { LOG_INFO("{}...", name_); }

    void print_outro(std::chrono::steady_clock::time_point now) {
        LOG_INFO("finished {}, took {}", name_, logger::format_duration(now - start_));
    }

    std::string name_;
    std::chrono::milliseconds min_interval_;
    bool real_time_;
    std::chrono::steady_clock::time_point start_ = std::chrono::steady_clock::now();
    std::atomic<bool> stopped_{false};
    std::thread thread_;
};

#endif // PHOSPHOR_LOGGER_HPP
