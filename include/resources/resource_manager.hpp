#pragma once

#include "core/globals.hpp"
#include "render/vulkan_types.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace dirk {

DECLARE_LOG_CATEGORY_EXTERN(LogResourceManager)

class DirkEngine;

struct ResourceManagerCreateInfo {
    DirkEngine* engine;
    const std::string& resourcePath;
    const std::string& shaderPath;
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
     * so no worries about reusing the function a lot.
     */
    std::shared_ptr<const Model> loadModel(const std::string& name);

    /**
     * Will load a shader file's bytes from:
     *   SHADER_PATH/<name>.spv
     *
     * This function also relies on a caching system and will only load a shader once,
     * so no worries about reusing this function a lot.
     */
    std::shared_ptr<const Shader> loadShader(const std::string& name);

private:
    std::unordered_map<std::string, std::weak_ptr<const Model>> models;
    std::unordered_map<std::string, std::weak_ptr<const Shader>> shaders;

    const std::string& resourcePath;
    const std::string& shaderPath;
};

} // namespace dirk
