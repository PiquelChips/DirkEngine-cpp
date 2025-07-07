#include "core/globals.hpp"
#include "engine/dirkengine.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>

int main() {
    try {
        auto engine = std::make_unique<DirkEngine>();

        GEngine = engine.get();

        return engine->main();
    } catch (std::exception e) {
        std::cerr << e.what();
        return EXIT_FAILURE;
    }
}
