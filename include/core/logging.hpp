#pragma once

#include <string>
#include <sstream>

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

const std::string logPath{ LOG_PATH };

/**
 * will log the log category, the log level and return a stream for the rest of the message
 * this will not actually output anything
 */
std::stringstream beginLogEntry(LogCategory category, LogLevel level);

/**
 * will actually output to the outputs (std::cout and/or a file)
 */
void endLogEntry(std::stringstream stream);

/**
 * return if a message should be logged based on level and the specific category
 */
bool shouldLog(LogCategory category, LogLevel level);

#define DECLARE_LOG_CATEGORY_EXTERN(categoryName) extern LogCategory categoryName;
#define DEFINE_LOG_CATEGORY(categoryName) LogCategory categoryName{ .name = #categoryName };

#define DIRK_LOG(category, level, messages)                      \
    if (shouldLog(category, level)) {                            \
        endLogEntry(beginLogEntry(category, level) << messages); \
    }

std::string getLevelString(LogLevel level);
std::string getLevelColor(LogLevel level);
