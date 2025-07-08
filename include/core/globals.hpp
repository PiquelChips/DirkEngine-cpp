#pragma once

#include "asserts.hpp"
#include "logging.hpp"

#define GLM_FORCE_RADIANS

class DirkEngine;

DECLARE_LOG_CATEGORY_EXTERN(LogTemp)

namespace dirk {

extern DirkEngine* gEngine;

} // namespace dirk
