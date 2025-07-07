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

#define DECLARE_LOG_CATEGORY_EXTERN(categoryName) extern LogCategory categoryName;
#define DEFINE_LOG_CATEGORY(categoryName) LogCategory categoryName{ .name = #categoryName };

#define DIRK_LOG(category, level) log(category, level)

int initializeLogger(const std::string& filename);
std::ostream& log(LogCategory category, LogLevel level);
