#include "core/logging.hpp"
#include "core/asserts.hpp"

#include <ctime>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <sstream>

constexpr std::string colorEnd{ "\033[0m" };

std::stringstream beginLogEntry(LogCategory category, LogLevel level) {
    std::stringstream stream{};

    std::string levelString = std::format("[{}{}{}] ", getLevelColor(level), getLevelString(level), colorEnd);
    char timestamp[25];
    time_t now = std::time(0);
    strftime(timestamp, sizeof(timestamp), "[%Y-%m-%d %H:%M:%S] ", localtime(&now));

    stream << timestamp << levelString << category.name << ": ";

    return stream;
}

void endLogEntry(std::stringstream stream) {
    stream << colorEnd;
    std::string out = stream.str();

    std::cout << out << std::endl;

    if (logPath == "")
        return;

    std::filesystem::create_directory(std::filesystem::path{ logPath });
    std::ofstream file(logPath + "/latest.log");
    check(file.is_open());

    file << out << std::endl;

    file.flush();
    file.close();

    // TODO: if fatal error, request engine exit
}

bool shouldLog(LogCategory category, LogLevel level) {
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

    return category.show;
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
        return "WARN";
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
