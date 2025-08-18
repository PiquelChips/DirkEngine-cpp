#include "engine/dirkengine.hpp"

int main(int argc, char** argv) {
    dirk::DirkEngineCreateInfo engineInfo = dirk::getEngineCreateInfo(argc, argv);
    auto engine = dirk::DirkEngine(engineInfo);
    return 0;
}
