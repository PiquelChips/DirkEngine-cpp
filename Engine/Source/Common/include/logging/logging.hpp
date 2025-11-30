#pragma once

#include <format>
#include <fstream>
#include <memory>
#include <string>

#define DECLARE_LOG_CATEGORY_EXTERN(categoryName) extern dirk::Logging::LogCategory categoryName;
#define DEFINE_LOG_CATEGORY(categoryName) dirk::Logging::LogCategory categoryName{ .name = #categoryName };

#define DIRK_LOG(category, level, message, ...) dirk::Logging::logger->log(category, dirk::Logging::level, message, ##__VA_ARGS__);

namespace dirk::Logging {

enum LogLevel {
    FATAL,
    ERROR,
    WARNING,
    INFO,
    DEBUG,
    TRACE
};

struct LogCategory {
    const char* name;
    bool show = true;
};

void init();
void shutdown();

class Logger {
public:
    Logger();
    ~Logger();

    void log(LogCategory category, LogLevel level, std::string str);

    template <typename... Args>
    void log(LogCategory category, LogLevel level, std::format_string<Args...> fmt, Args&&... args) {
        if (shouldLog(category, level))
            return;

        log(category, level, std::vformat(fmt.get(), std::make_format_args(args...)));
    }

    static bool shouldLog(LogCategory category, LogLevel level);

private:
    std::string filepath;
    std::ofstream logfile;

    static constexpr std::string_view logPath{ LOG_PATH };
};

static std::unique_ptr<Logger> logger = nullptr;

} // namespace dirk::Logging
