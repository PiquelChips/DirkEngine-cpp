#pragma once

namespace dirk {

struct ActorSpawnInfo {
    const std::string& name;
    DirkEngine* engine;
};

/**
 * Just the representation of something in the game world
 *
 * TODO: create a world class to manage this stuff (instead of engine)
 */
class Actor {

public:
    Actor(ActorSpawnInfo& spawnInfo);

    void destroy();

    const std::string& getName() const { return name; }

    // begin Actor interface
public:
    virtual void beginPlay();
    virtual void tick(float deltaTime);
    virtual void endPlay();
    // end Actor interface
private:
    DirkEngine* engine;
    const std::string& name;

} // namespace dirk
