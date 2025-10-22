#include "resources/resource_manager.hpp"

#include "tinygltf.h"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace dirk {

DEFINE_LOG_CATEGORY(LogResourceManager)

std::shared_ptr<const Model> ResourceManager::loadModel(const std::string& name) {
    if (models.contains(name)) {
        if (std::shared_ptr<const Model> model = models[name].lock()) {
            check(model->name == name);
            return model;
        } else {
            models.erase(name);
        }
    }

    // load the model
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string warn, err;

    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, std::string(resourcePath) + "/models/" + name + "/" + name + ".gltf");

    if (warn != "") {
        warn.pop_back(); // remove trailing return
        DIRK_LOG(LogResourceManager, WARNING, "tinygltf: " << warn);
    }
    if (err != "") {
        err.pop_back(); // remove trailing return
        DIRK_LOG(LogResourceManager, ERROR, "tinygltf: " << err);
    }

    if (!ret) {
        DIRK_LOG(LogResourceManager, FATAL, "failed to load model: " << name);
        return nullptr;
    }

    // TODO: reserve the vertex and index vectors
    std::vector<Vertex> vertices{};
    std::vector<std::uint32_t> indices{};

    std::unordered_map<Vertex, std::uint32_t> uniqueVertices{};

    // TODO: take more than just the first of both
    const auto& mesh = model.meshes[0];
    const auto& primitive = mesh.primitives[0];

    // vertex positions
    const tinygltf::Accessor& posAccessor = model.accessors[primitive.attributes.at("POSITION")];
    const tinygltf::BufferView& posBufferView = model.bufferViews[posAccessor.bufferView];
    const tinygltf::Buffer& posBuffer = model.buffers[posBufferView.buffer];

    // tex coords
    bool hasTexCoords = primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end();
    const tinygltf::Accessor* texCoordAccessor = hasTexCoords ? &model.accessors[primitive.attributes.at("TEXCOORD_0")] : nullptr;
    const tinygltf::BufferView* texCoordBufferView = hasTexCoords ? &model.bufferViews[texCoordAccessor->bufferView] : nullptr;
    const tinygltf::Buffer* texCoordBuffer = hasTexCoords ? &model.buffers[texCoordBufferView->buffer] : nullptr;

    for (std::size_t i = 0; i < posAccessor.count; i++) {
        Vertex vertex{};

        const float* pos = reinterpret_cast<const float*>(&posBuffer.data[posBufferView.byteOffset + posAccessor.byteOffset + i * 12]);
        vertex.pos = { pos[0], pos[1], pos[2] };

        vertex.texCoord = { 0.f, 0.f };
        if (hasTexCoords) {
            const float* texCoord = reinterpret_cast<const float*>(&texCoordBuffer->data[texCoordBufferView->byteOffset + texCoordAccessor->byteOffset + i * 8]);
            vertex.texCoord = { texCoord[0], texCoord[1] };
        }

        vertex.color = { 1.f, 1.f, 1.f };
        vertices.push_back(vertex);
    }

    // indices
    const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
    const tinygltf::BufferView& indexBufferView = model.bufferViews[indexAccessor.bufferView];
    const tinygltf::Buffer& indexBuffer = model.buffers[indexBufferView.buffer];

    switch (indexAccessor.componentType) {
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
        const uint8_t* indexData = reinterpret_cast<const uint8_t*>(&indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset]);
        for (size_t i = 0; i < indexAccessor.count; i++) {
            indices.push_back(indexData[i]);
        }
        break;
    }
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
        const uint16_t* indexData = reinterpret_cast<const uint16_t*>(&indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset]);
        for (size_t i = 0; i < indexAccessor.count; i++) {
            indices.push_back(indexData[i]);
        }
        break;
    }
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
        const uint32_t* indexData = reinterpret_cast<const uint32_t*>(&indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset]);
        for (size_t i = 0; i < indexAccessor.count; i++) {
            indices.push_back(indexData[i]);
        }
        break;
    }
    }

    // texture
    const tinygltf::Material& material = model.materials[primitive.material];
    const tinygltf::Texture& texture = model.textures[material.pbrMetallicRoughness.baseColorTexture.index];
    const tinygltf::Image& textureImage = model.images[texture.source];

    std::shared_ptr<Model> modelPtr = std::make_shared<Model>(Model{
        .name = name,
        .vertices = vertices,
        .indices = indices,
        .texture = Texture{
            .width = static_cast<uint32_t>(textureImage.width),
            .height = static_cast<uint32_t>(textureImage.height),
            .size = static_cast<uint32_t>(textureImage.width * textureImage.height * (textureImage.bits / 8) * 4),
            .texture = textureImage.image,
        },
    });

    models[name] = modelPtr;
    return modelPtr;
}

std::shared_ptr<const Shader> ResourceManager::loadShader(const std::string& name) {
    if (shaders.contains(name)) {
        if (std::shared_ptr<const Shader> shader = shaders[name].lock()) {
            check(shader->name == name);
            return shader;
        } else {
            shaders.erase(name);
        }
    }

    // load the shader
    std::ifstream file(std::string(shaderPath) + "/" + name + ".spv", std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        DIRK_LOG(LogResourceManager, FATAL, "unable to load shader: " << name)
        return nullptr;
    }

    size_t fileSize = file.tellg();
    std::vector<char> shader(fileSize);

    file.seekg(0);
    file.read(shader.data(), fileSize);
    file.close();

    std::shared_ptr<Shader>
        shaderPtr = std::make_shared<Shader>(Shader{
            .name = name,
            .size = fileSize,
            .shader = shader,
        });

    shaders[name] = shaderPtr;
    return shaderPtr;
}

} // namespace dirk
