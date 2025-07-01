#pragma once

#include "vulkan/vulkan_core.h"
#include <string>
#include <vector>

class ShaderManager {
public:
    static VkShaderModule loadShaderModule(VkDevice device, const std::string& shaderName);

private:
    static std::vector<char> readFile(const std::string& filename);
};
