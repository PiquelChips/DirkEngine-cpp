#include "engine/shaders.hpp"
#include "vulkan/vulkan_core.h"

#include <cassert>
#include <cstdint>
#include <fstream>
#include <ios>
#include <string>
#include <vector>

VkShaderModule ShaderManager::loadShaderModule(VkDevice device, const std::string& shaderName) {
    auto shader = readFile(std::string(SHADER_PATH) + "/" + shaderName + ".spv");

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shader.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(shader.data());

    VkShaderModule shaderModule;
    assert(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) == VK_SUCCESS);
    return shaderModule;
}

std::vector<char> ShaderManager::readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    assert(file.is_open());

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}
