#include "core/logging.hpp"

#include <cstdarg>
#include <cstdlib>
#include <ctime>
#include <format>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>

constexpr std::string colorEnd{ "\033[0m" };

std::stringstream beginLogEntry(LogCategory category, LogLevel level) {
    std::stringstream stream{};

    // TODO: log to file
    // file.open(filename);

    std::string levelString = std::format("[{}{}{}]", getLevelColor(level), getLevelString(level), colorEnd);
    char timestamp[25];
    time_t now = std::time(0);
    strftime(timestamp, sizeof(timestamp), "[%Y-%m-%d %H:%M:%S] ", localtime(&now));

    stream << timestamp << levelString << ": ";

    // if (file.is_open())
    //     file << outString;

    return stream;
}

void endLogEntry(std::stringstream stream) {
    stream << colorEnd << "\n";

    std::cout << stream.str();
}

bool shouldLog(LogCategory category, LogLevel level) {
    if (!category.show)
        return false;

    switch (level) {
#ifndef DEBUG_BUILD
    case DEBUG:
        return false;
    case TRACE:
        return false;
#endif
    default:
#ifdef NO_LOGGING
        return false;
#else
        break;
#endif
    }

    return true;
}

std::string getLevelString(LogLevel level) {
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
    }
    return "";
}

std::string getLevelColor(LogLevel level) {
    std::string color{ "\033[" };

    switch (level) {
    case TRACE:
        color += "36"; // cyan
        break;
    case DEBUG:
        color += "34"; // blue
        break;
    case INFO:
        color += "32"; // green
        break;
    case WARNING:
        color += "33"; // yellow
        break;
    case ERROR:
        color += "31"; // red
        break;
    case FATAL:
        color += "35"; // magenta
        break;
    default:
        return "";
    }

    color += "m";
    return color;
}
