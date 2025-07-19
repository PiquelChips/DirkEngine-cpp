#pragma once

namespace dirk {

/**
 * This class represents a game build using the engine.
 *
 * All game startup and shutdown functionnality lives here.
 * To make your own game you must create a child class and implement
 * the interface.
 */
class GameInstance {
public:
    virtual int initialize() = 0;
    virtual void begin() = 0;
    virtual void deinitialize() = 0;
};

} // namespace dirk
