#include "engine/dirkengine.hpp"

int main(int argc, char** argv) {
    dirk::DirkEngineCreateInfo engineInfo = dirk::getEngineCreateInfo(argc, argv);

    return dirk::DirkEngine(engineInfo).run();
}
