#pragma once

#include <cassert>

#define check(expr) assert(expr)
#define checkVulkan(expr) assert(expr == vk::Result::eSuccess)
