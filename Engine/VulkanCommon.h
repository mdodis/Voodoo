#pragma once
#include <vulkan/vulkan.h>

#include "Compat/Win32Base.h"
#include "Containers/Slice.h"
#include "Result.h"

#define VK_RETURN_IF_ERR(x)           \
    do {                              \
        VkResult __result = (x);      \
        if (__result != VK_SUCCESS) { \
            return Err(__result);     \
        }                             \
    } while (0)

#define VK_PROC_DEBUG_CALLBACK(name)                               \
    VkBool32 VKAPI_CALL name(                                      \
        VkDebugUtilsMessageSeverityFlagBitsEXT      severity,      \
        VkDebugUtilsMessageTypeFlagsEXT             type,          \
        const VkDebugUtilsMessengerCallbackDataEXT *callback_data, \
        void                                       *user_data)

Slice<const char *> win32_get_required_extensions();

Result<VkSurfaceKHR, VkResult> win32_create_surface(
    VkInstance vk_instance, Win32::HWND hwnd, Win32::HINSTANCE instance);