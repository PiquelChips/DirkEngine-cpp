#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES // properly aligns types in memory; this doesn't affect nested structs!!!
#define GLM_FORCE_DEPTH_ZERO_TO_ONE        // vulkan uses the 0.0 to 1.0 ranges; opengl uses the -1.0 to 1.0 range
#define GLM_ENABLE_EXPERIMENTAL            // for the glm hash functions

#include "glm/glm.hpp"

#include "asserts.hpp"
#include "logging.hpp"
