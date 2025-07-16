#pragma once

#include "core/globals.hpp"
#include "render/render_types.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace dirk {

DECLARE_LOG_CATEGORY_EXTERN(LogResourceManager)

class DirkEngine;

struct ResourceManagerCreateInfo {
    DirkEngine* engine;
    const std::string& resourcePath;
};

/**
 * The class to manage all of the engine's filesystem operations.
 * These include loading/unloading shaders, models, config files, and any other data
 * needed for the engine to run.
 */
class ResourceManager {

public:
    ResourceManager(ResourceManagerCreateInfo& createInfo);

    /**
     * Will load a model with the filepath:
     *   RESOURCE_PATH/models/<name>/<name>.gltf
     *
     * This function relies on a caching system and will only load a model once,
     * so now worries about reusing the function a lot.
     */
    std::shared_ptr<Model> loadModel(const std::string& name);

private:
    std::unordered_map<std::string, std::weak_ptr<Model>> models;

    const std::string& resourcePath;
};

} // namespace dirk
