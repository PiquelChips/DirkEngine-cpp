#include "resources/resource_manager.hpp"

#include <memory>

namespace dirk {

std::shared_ptr<Model> ResourceManager::loadModel(const std::string& name) {
    if (models.contains(name)) {
        if (std::shared_ptr<Model> model = models[name].lock())
            return model;
        else
            models.erase(name);
    }
}

void ResourceManager::unloadModel(const std::string& name) {
    models.erase(name);
}

} // namespace dirk
