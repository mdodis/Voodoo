#pragma once
#include <vulkan/vulkan.h>
#include "Result.h"

#define VK_RETURN_IF_ERR(x) do {  \
    VkResult __result = (x);      \
    if (__result != VK_SUCCESS) { \
        return Err(__result);     \
    }                             \
    } while(0)