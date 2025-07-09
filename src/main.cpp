#include "core/globals.hpp"
#include "engine/dirkengine.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "thirdparty/stb_image.h"

DEFINE_LOG_CATEGORY(LogTemp)

int main() {
    try {
        auto engine = std::make_unique<DirkEngine>();
        dirk::gEngine = engine.get();

        return dirk::gEngine->main();
    } catch (std::exception e) {
        std::cerr << e.what();
        return EXIT_FAILURE;
    }
}
