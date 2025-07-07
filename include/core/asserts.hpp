#pragma once

#include <cassert>

#define check(expr) assert(expr)

#define checkVulkan(expr) check(expr == vk::Result::eSuccess)
