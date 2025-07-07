#pragma once

#include <string>

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

std::stringstream beginLogEntry(LogCategory category, LogLevel level);
void endLogEntry(std::stringstream stream);

#define DECLARE_LOG_CATEGORY_EXTERN(categoryName) extern LogCategory categoryName;
#define DEFINE_LOG_CATEGORY(categoryName) LogCategory categoryName{ .name = #categoryName };

// #define DIRK_LOG(category, level) log(category, level)
#define DIRK_LOG(category, level, messages) \
    endLogEntry(beginLogEntry(category, level) << messages);

std::string getLevelString(LogLevel level);
std::string getLevelColor(LogLevel level);
