#pragma once
#include <vulkan/vulkan.h>
#include "Result.h"
#include "Containers/Slice.h"

#define VK_RETURN_IF_ERR(x) do {  \
    VkResult __result = (x);      \
    if (__result != VK_SUCCESS) { \
        return Err(__result);     \
    }                             \
    } while(0)

#define VK_PROC_DEBUG_CALLBACK(name) VkBool32 VKAPI_CALL name( \
    VkDebugUtilsMessageSeverityFlagBitsEXT severity, \
    VkDebugUtilsMessageTypeFlagsEXT type, \
    const VkDebugUtilsMessengerCallbackDataEXT *callback_data, \
    void *user_data)

Slice<const char*>get_win32_required_extensions();