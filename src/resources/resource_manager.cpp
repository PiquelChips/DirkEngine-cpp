#include "resources/resource_manager.hpp"

#include <memory>

namespace dirk {

ResourceManager::ResourceManager(ResourceManagerCreateInfo& createInfo) : resourcePath(createInfo.resourcePath) {};

std::shared_ptr<Model> ResourceManager::loadModel(const std::string& name) {
    if (models.contains(name)) {
        if (std::shared_ptr<Model> model = models[name].lock())
            return model;
        else
            models.erase(name);
    }
}

} // namespace dirk
