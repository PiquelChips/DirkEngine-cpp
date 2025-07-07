#include "core/logging.hpp"
#include <cstdlib>
#include <iostream>
#include <string>

int initializeLogger(const std::string& filename) {
    // TODO: actually initialize logger (so open file and stuff)
    return EXIT_SUCCESS;
}

std::ostream& log(LogCategory category, LogLevel level) {
    if (!category.show)
        return std::cout;

    switch (level) {
#ifndef DEBUG_BUILD
    case DEBUG:
        return std::cout;
    case TRACE:
        return std::cout;
#endif
    default:
#ifdef NO_LOGGING
        return std::cout;
#else
        break;
#endif
    }

    // TODO: actually log the stuff

    return std::cout;
}
