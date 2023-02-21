#include "VulkanCommon.h"
#include <Windows.h>
#include <vulkan/vulkan_win32.h>

auto Win32_Required_Extensions = arr<const char*>(
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME
);

Slice<const char*> get_win32_required_extensions() {
    return slice(Win32_Required_Extensions);
}
