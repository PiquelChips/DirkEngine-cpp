#pragma once

#include <string>

namespace dirk {

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

#define DEFINE_LOG_CATEGORY(categoryName) LogCategory categoryName{ .name = #categoryName };

// define some basic log categories
DEFINE_LOG_CATEGORY(LogTemp)
DEFINE_LOG_CATEGORY(LogDirkCore)

#define DIRK_LOG(category, level, message) log(category, level, const std::string& message);

int initializeLogger(const std::string& filename);
void log(LogCategory category, LogLevel level, const std::string& message);

} // namespace dirk
