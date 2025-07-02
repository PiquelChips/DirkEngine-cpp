#include "engine/dirkengine.hpp"

#include <cstdlib>

int main() {
    auto engine = std::make_unique<DirkEngine>();
    return engine->main();
}
