#include "core/logging.hpp"

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>

#define NO_LOG Log(false);

Log::Log(bool shouldLog) : shouldLog(shouldLog), buffer(std::cout) {}

int initializeLogger(const std::string& filename) {
    // TODO: actually initialize logger (so open file and stuff)
    return EXIT_SUCCESS;
}

Log log(LogCategory category, LogLevel level) {
    if (!category.show)
        return NO_LOG;

    switch (level) {
#ifndef DEBUG_BUILD
    case DEBUG:
        return NO_LOG;
    case TRACE:
        return NO_LOG;
#endif
    default:
#ifdef NO_LOGGING
        return NO_LOG;
#else
        break;
#endif
    }

    Log log(true);

    // TODO: log to file
    // file.open(filename);

    std::string levelString{ GetLevelString(level) };
    char timestamp[25];
    time_t now = std::time(0);
    strftime(timestamp, sizeof(timestamp), "[%Y-%m-%d %H:%M:%S] ",
             localtime(&now));

    log << "\n"
        << timestamp << levelString << ": ";

    // if (file.is_open())
    //     file << outString;

    return log;
}

std::string GetLevelString(LogLevel level) {
    switch (level) {
    case TRACE:
        return "TRACE";
    case DEBUG:
        return "DEBUG";
    case INFO:
        return "INFO";
    case WARNING:
        return "WARNING";
    case ERROR:
        return "ERROR";
    case FATAL:
        return "FATAL";
    default:
        return "";
    }
}
