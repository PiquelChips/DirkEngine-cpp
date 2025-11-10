#ifdef PLATFORM_LINUX

#include "platform/linux/window.hpp"
#include "logging/logging.hpp"

#include "wayland-client-core.h"

namespace dirk::Platform::Linux {

DEFINE_LOG_CATEGORY(LogLinux)
DEFINE_LOG_CATEGORY(LogWayland)

std::vector<const char*> getRequiredExtensions() {}

LinuxWindow::LinuxWindow() {}
LinuxWindow::~LinuxWindow() {}

void* LinuxWindow::getNativeHandle() {}

vk::SurfaceKHR LinuxWindow::createVulkanSurface(vk::Instance instance) {}

vk::Extent2D LinuxWindow::getSize() {}
void LinuxWindow::setSize(vk::Extent2D inSize) {}
vk::Extent2D LinuxWindow::getFramebufferSize() {}

glm::vec2 LinuxWindow::getPosition() {}
void LinuxWindow::setPosition(const glm::vec2 inPosition) {}

std::string_view LinuxWindow::getTitle() {}
void LinuxWindow::setTitle(std::string_view inTitle) {}

bool LinuxWindow::isFocused() {}
bool LinuxWindow::isMinimized() {}

} // namespace dirk::Platform::Linux

#endif
