#pragma once
#define MOK_WIN32_NO_FUNCTIONS
#include "Compat/Win32Base.h"
#include "vulkan/vulkan.h"
#include "Result.h"
#include "Base.h"

namespace win {
    Slice<const char*>             win32_get_required_extensions();
    Result<VkSurfaceKHR, VkResult> win32_create_surface(
        VkInstance vk_instance, Win32::HWND hwnd, Win32::HINSTANCE instance);
}
