#pragma once

#include "logging/logging.hpp"

#include <cassert>
#include <cmath>
#include <format>

#define checkm(expr, message, ...)                                                           \
    if (!(expr)) {                                                                           \
        dirk::Logging::logger->log(                                                          \
            dirk::Logging::LogCategory{ .name = "Assertion Failed" },                        \
            dirk::Logging::FATAL,                                                            \
            "{}:{} {}\n{}", __FILE__, __LINE__, #expr, std::format(message, ##__VA_ARGS__)); \
    }

#define check(expr) checkm(expr, "")

#define checkVulkan(expr) check(expr == vk::Result::eSuccess)
