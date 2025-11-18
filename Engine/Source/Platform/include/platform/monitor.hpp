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

    glm::vec2 getPosition() { return position; }
    void setPosition(glm::vec2 inPosition) { position = inPosition; }

    // TODO: some function to convert to ImGui monitor

private:
    void* platformHandle;
    std::string_view name;
    glm::vec2 position;
    VideoMode videoMode;
    Platform& platform;
};

} // namespace dirk::Platform
