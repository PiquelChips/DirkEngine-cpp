#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

#include_next "glm/glm.hpp"

#include "asserts.hpp"
#include "logging.hpp"

class DirkEngine;

DECLARE_LOG_CATEGORY_EXTERN(LogTemp)

namespace dirk {

extern DirkEngine* gEngine;

} // namespace dirk
