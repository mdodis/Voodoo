#include "Win32Vulkan.h"
#include <Windows.h>
#include <vulkan/vulkan_win32.h>
#include "Containers/Array.h"
#include "VulkanCommon.h"

namespace win {

    auto Win32_Required_Extensions = arr<const char*>(
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

    Slice<const char*> win32_get_required_extensions()
    {
        return slice(Win32_Required_Extensions);
    }

    Result<VkSurfaceKHR, VkResult> win32_create_surface(
        VkInstance vk_instance, Win32::HWND hwnd, Win32::HINSTANCE instance)
    {
        VkWin32SurfaceCreateInfoKHR create_info = {
            .sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            .hinstance = (HINSTANCE)instance,
            .hwnd      = (HWND)hwnd,
        };

        VkSurfaceKHR result;
        VK_RETURN_IF_ERR(
            vkCreateWin32SurfaceKHR(vk_instance, &create_info, 0, &result));
        return Ok(result);
    }

}
