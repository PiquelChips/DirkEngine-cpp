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

#define DEFINE_LOG_CATEGORY(categoryName) LogCategory categoryName{ .name = #categoryName };
#define DIRK_LOG(category, level, message) log(category, level, message);

int initializeLogger(const std::string& filename);
void log(LogCategory category, LogLevel level, const std::string& message);
