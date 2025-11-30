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

template <typename... Args>
void Logger::log(LogCategory category, LogLevel level, std::format_string<Args...> fmt, Args&&... args) {
    if (shouldLog(category, level))
        return;

    log(category, level, std::vformat(fmt.get(), std::make_format_args(args...)));
}

void Logger::log(LogCategory category, LogLevel level, std::string message) {
    if (shouldLog(category, level))
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
        levelColoredString += "\033[36mTRACE\033[0m"; // cyan
        break;
    case DEBUG:
        levelString += "DEBUG";
        levelColoredString += "\033[34mDEBUG\033[0m"; // blue
        break;
    case INFO:
        levelString += "INFO";
        levelColoredString += "\033[32mINFO\033[0m"; // green
        break;
    case WARNING:
        levelString += "WARN";
        levelColoredString += "\033[33mWARN\033[0m"; // yellow
        break;
    case ERROR:
        levelString += "ERROR";
        levelColoredString += "\033[31mERROR\033[0m"; // red
        break;
    case FATAL:
        levelString += "FATAL";
        levelColoredString += "\033[35mFATAL\033[0m"; // magenta
        break;
    }

    levelString += "]";
    levelColoredString = "]";

    logfile << timeStr << " " << levelString << " " << message;
    logfile.flush();

    std::println(std::cout, "{} {} {}", timeStr, levelColoredString, message);
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
