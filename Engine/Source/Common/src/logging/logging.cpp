#include "logging/logging.hpp"
#include "asserts.hpp"

#include <bits/chrono.h>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <ostream>

namespace dirk::Logging {

void init() {
    logger = std::make_unique<Logger>();
}

void shutdown() {
    logger = nullptr;
}

Logger::Logger() {
    // TODO: create parent directories as well
    std::filesystem::create_directory(std::filesystem::path{ logPath });
    logfile = std::ofstream(std::format("{}/latest.log", logPath), std::ios::out | std::ios::app);
    check(logfile.is_open());
}

Logger::~Logger() {
    logfile.flush();
    logfile.close();
}

static std::string makeColoredMessage(int color, const std::string& message) {
    return std::format("\033[{}m{}\033[0m", color, message);
}

void Logger::log(LogCategory category, LogLevel level, std::string message) {
    if (!shouldLog(category, level))
        return;

    static auto currentZone = std::chrono::current_zone();
    const auto zonedTime = std::chrono::zoned_time{ currentZone, std::chrono::system_clock::now() };
    const auto time = std::chrono::hh_mm_ss(zonedTime.get_local_time() - std::chrono::floor<std::chrono::days>(zonedTime.get_local_time()));
    std::string timeStr = std::format("[{}]", time);

    std::string levelString = "[";
    std::string levelColoredString = "[";

    switch (level) {
    case TRACE:
        levelString += "TRACE";
        levelColoredString += makeColoredMessage(36, "TRACE"); // cyan
        break;
    case DEBUG:
        levelString += "DEBUG";
        levelColoredString += makeColoredMessage(34, "TRACE"); // blue
        break;
    case INFO:
        levelString += "INFO";
        levelColoredString += makeColoredMessage(32, "INFO"); // green
        break;
    case WARNING:
        levelString += "WARN";
        levelColoredString += makeColoredMessage(33, "WARN"); // yellow
        break;
    case ERROR:
        levelString += "ERROR";
        levelColoredString += makeColoredMessage(31, "ERROR"); // red
        break;
    case FATAL:
        levelString += "FATAL";
        levelColoredString += makeColoredMessage(35, "FATAL"); // magenta
        break;
    }

    levelString += "]";
    levelColoredString += "]";

    std::string msg = std::format("{} {} {} {}", time, levelString, category.name, message);
    std::string coloredMsg = std::format("{} {} {} {}", time, levelString, category.name, message);

    // why tf does this line segfault
    logfile << msg << std::endl;
    std::cout << coloredMsg << std::endl;
}

bool Logger::shouldLog(LogCategory category, LogLevel level) {
#ifdef NO_LOGGING
    return false;
#endif

    switch (level) {
#ifndef DIRK_DEBUG_BUILD
    case DEBUG:
        return false;
    case TRACE:
        return false;
#endif
    default:
        break;
    }

    return category.show;
}

} // namespace dirk::Logging
