#include "logging/logging.hpp"
#include "asserts.hpp"

#include <chrono>
#include <csignal>
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
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path{ logPath }, ec);

    if (ec) {
        std::cerr << "Failed to create log directories: " << ec.message() << std::endl;
    }

    logfile = std::ofstream(std::format("{}/latest.log", logPath), std::ios::out | std::ios::trunc);
    check(logfile.is_open());
}

Logger::~Logger() {
    check(logfile.is_open());
    logfile.flush();
    logfile.close();

    try {
        auto now = std::chrono::system_clock::now();
        std::string timestampedName = std::format("{}/log_{:%Y-%m-%d_%H-%M-%S}.log", logPath, now);

        std::filesystem::path source = std::format("{}/latest.log", logPath);
        std::filesystem::path target = timestampedName;

        std::filesystem::copy_file(source, target, std::filesystem::copy_options::overwrite_existing);
    } catch (const std::exception& e) {
        std::cerr << "Failed to archive log file: " << e.what() << std::endl;
    }
}

static std::string makeColoredMessage(int color, const std::string& message) {
    return std::format("\033[{}m{}\033[0m", color, message);
}

void Logger::log(LogCategory category, LogLevel level, std::string message) {
    if (!shouldLog(category, level))
        return;

    static auto currentZone = std::chrono::current_zone();
    auto time = std::chrono::zoned_time{ currentZone, std::chrono::system_clock::now() };
    time = std::chrono::floor<std::chrono::seconds>(time.get_local_time());
    std::string timeStr = std::format("{:%Y/%m/%d %H:%M:%S}", time);
    timeStr = timeStr.substr(0, 19);

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

    std::string msg = std::format("{} {} {} {}", timeStr, levelString, category.name, message);
    std::string coloredMsg = std::format("{} {} {} {}", timeStr, levelColoredString, category.name, message);

    // TODO: fix segfault
    // std::println(logfile, "{}", msg);
    std::println(std::cout, "{}", coloredMsg);

    if (level == FATAL) {
        shutdown();
        std::abort();
    }
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
