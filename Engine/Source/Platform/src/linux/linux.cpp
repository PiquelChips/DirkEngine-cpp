#include <vector>

#ifdef PLATFORM_LINUX

namespace dirk::Platform {

std::vector<const char*> getRequiredExtensions() {
    return std::vector<const char*>();
}

} // namespace dirk::Platform

#endif
