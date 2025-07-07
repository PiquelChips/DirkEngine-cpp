#pragma once

#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>
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

struct Log {
public:
    Log(bool shouldLog = true);

    template <typename T>
    Log operator<<(const T& value) {
        if (shouldLog)
            buffer << value;

        return *this;
    }

private:
    std::ostream& buffer;
    bool shouldLog;
};

#define DECLARE_LOG_CATEGORY_EXTERN(categoryName) extern LogCategory categoryName;
#define DEFINE_LOG_CATEGORY(categoryName) LogCategory categoryName{ .name = #categoryName };

#define DIRK_LOG(category, level) log(category, level)

Log log(LogCategory category, LogLevel level);
std::string GetLevelString(LogLevel level);
