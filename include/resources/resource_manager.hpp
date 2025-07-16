#pragma once

#include "core/globals.hpp"
#include "render/render_types.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

namespace dirk {

/**
 * The class to manage all of the engine's filesystem operations.
 * These include loading/unloading shaders, models, config files, and any other data
 * needed for the engine to run.
 */
class ResourceManager {

public:
    /**
     * Will load a model with the filepath:
     *   RESOURCE_PATH/models/<name>/<name>.gltf
     *
     * This function relies on a caching system and will only load a model once,
     * so now worries about reusing the function a lot.
     */
    std::shared_ptr<Model> loadModel(const std::string& name);
    void unloadModel(const std::string& name);

private:
    std::unordered_map<std::string, std::weak_ptr<Model>> models;
};

} // namespace dirk
