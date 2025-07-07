#include "core/logging.hpp"
#include <cstdlib>
#include <string>

int initializeLogger(const std::string& filename) {
    // TODO: actually initialize logger (so open file and stuff)
    return EXIT_SUCCESS;
}

void log(LogCategory category, LogLevel level, const std::string& message) {
    if (!category.show)
        return;

    switch (level) {
#ifndef DEBUG_BUILD
    case DEBUG:
        return;
    case TRACE:
        return;
#endif
    default:
#ifdef NO_LOGGING
        return;
#else
        break;
#endif
    }

    // TODO: actually log the stuff
}
