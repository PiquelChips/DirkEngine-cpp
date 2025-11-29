#pragma once

#include "glm/glm.hpp"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_structs.hpp"

#include <cstdint>
#include <string_view>

namespace dirk::Platform {

struct VideoMode {
    vk::Extent2D size = { 0, 0 };
    std::uint32_t red, green, blue = 8;
    std::uint32_t refreshRate = 60;
};

class Platform;

class Monitor {
public:
    Monitor(void* platformHandle, Platform& platform) : platformHandle(platformHandle), platform(platform) {}

    void* getPlatformHandle() { return platformHandle; }
    Platform& getPlatform() { return platform; }

    VideoMode& getVideoMode() { return videoMode; }
    void setVideoMode(VideoMode& inVideoMode) { videoMode = inVideoMode; }

    std::string_view getName() { return name; }
    void setName(std::string_view inName) { name = inName; }

    std::string_view getMake() { return make; }
    void setMake(std::string_view inMake) { make = inMake; }

    std::string_view getModel() { return model; }
    void setModel(std::string_view inModel) { model = inModel; }

    std::string_view getDescription() { return description; }
    void setDescription(std::string_view inDescription) { description = inDescription; }

    glm::vec2 getPosition() { return position; }
    void setPosition(glm::vec2 inPosition) { position = inPosition; }

    // TODO: some function to convert to ImGui monitor

private:
    std::string_view name;
    std::string_view make;
    std::string_view model;
    std::string_view description;

    void* platformHandle;
    glm::vec2 position;
    VideoMode videoMode;
    Platform& platform;
};

} // namespace dirk::Platform
