#define MOK_WIN32_NO_FUNCTIONS
#include "VulkanCommon.h"

#include <Windows.h>
#include <vulkan/vulkan_win32.h>

#include <set>

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

bool validation_layers_exist(IAllocator& allocator, Slice<const char*> layers)
{
    TArray<VkLayerProperties> props(&allocator);
    DEFER(props.release());

    u32 layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, 0);

    props.init_range(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, props.data);

    for (const char* layer : layers) {
        bool found = false;

        for (const auto& prop : props) {
            if (strcmp(prop.layerName, layer) == 0) {
                found = true;
                break;
            }
        }

        if (!found) {
            return false;
        }
    }

    return true;
}

Result<VkPhysicalDevice, VkResult> pick_physical_device(
    IAllocator& allocator, VkInstance instance, PickPhysicalDeviceInfo& info)
{
    u32 device_count;
    VK_RETURN_IF_ERR(vkEnumeratePhysicalDevices(instance, &device_count, 0));

    TArray<VkPhysicalDevice> physical_devices(&allocator);
    physical_devices.init_range(device_count);
    DEFER(physical_devices.release());

    VK_RETURN_IF_ERR(vkEnumeratePhysicalDevices(
        instance,
        &device_count,
        physical_devices.data));

    int best_device       = -1;
    int best_device_score = -1;
    for (int i = 0; i < device_count; ++i) {
        VkPhysicalDevice device       = physical_devices[i];
        int              device_score = 0;

        // Get properties that determine score
        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceFeatures   features;

        vkGetPhysicalDeviceProperties(device, &properties);
        vkGetPhysicalDeviceFeatures(device, &features);

        u32 num_extensions;
        vkEnumerateDeviceExtensionProperties(device, 0, &num_extensions, 0);

        TArray<VkExtensionProperties> extensions(&allocator);
        extensions.init_range(num_extensions);
        DEFER(extensions.release());
        vkEnumerateDeviceExtensionProperties(
            device,
            0,
            &num_extensions,
            extensions.data);

        // Check for extension support
        bool supports_extensions = true;
        for (const char* required_extension : info.device_extentions) {
            bool found = false;
            for (VkExtensionProperties& props : extensions) {
                if (strcmp(props.extensionName, required_extension) == 0) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                supports_extensions = false;
                break;
            }
        }

        if (!supports_extensions) {
            // If it doesn't support the required extensions, don't examine
            // further
            continue;
        }

        auto swap_chain_support_result =
            query_physical_device_swap_chain_support(
                allocator,
                device,
                info.surface);

        if (!swap_chain_support_result.ok())
            return Err(swap_chain_support_result.err());

        SwapChainSupportInfo swap_chain_support =
            swap_chain_support_result.value();

        if ((swap_chain_support.formats.size == 0) ||
            (swap_chain_support.present_modes.size == 0))
        {
            // If it doesn't support swap chain with any formats or present
            // modes, don't examine further
            continue;
        }

        // Rank device type
        if (properties.deviceType == info.prefered_device_type) {
            device_score += 10;
        }

        // Check if this is better than the best device available
        if (device_score > best_device_score) {
            best_device       = i;
            best_device_score = device_score;
        }
    }

    if (best_device < 0) {
        return Err(VK_ERROR_UNKNOWN);
    }

    VkPhysicalDevice ret_device = physical_devices[best_device];

    return Ok(ret_device);
}

Result<SwapChainSupportInfo, VkResult> query_physical_device_swap_chain_support(
    IAllocator& allocator, VkPhysicalDevice device, VkSurfaceKHR surface)
{
    SwapChainSupportInfo result(allocator);

    VK_RETURN_IF_ERR(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        device,
        surface,
        &result.capabilities));

    u32 format_count = 0;
    VK_RETURN_IF_ERR(vkGetPhysicalDeviceSurfaceFormatsKHR(
        device,
        surface,
        &format_count,
        0));

    if (format_count != 0) {
        result.formats.init_range(format_count);
        VK_RETURN_IF_ERR(vkGetPhysicalDeviceSurfaceFormatsKHR(
            device,
            surface,
            &format_count,
            result.formats.data));
    }

    u32 present_mode_count = 0;
    VK_RETURN_IF_ERR(vkGetPhysicalDeviceSurfacePresentModesKHR(
        device,
        surface,
        &present_mode_count,
        0));

    if (present_mode_count != 0) {
        result.present_modes.init_range(present_mode_count);
        VK_RETURN_IF_ERR(vkGetPhysicalDeviceSurfacePresentModesKHR(
            device,
            surface,
            &present_mode_count,
            result.present_modes.data));
    }

    return Ok(result);
}

Result<VkDevice, VkResult> create_device_with_queues(
    IAllocator& allocator, CreateDeviceWithQueuesInfo& info)
{
    info.families.alloc = &allocator;
    info.families.init(info.family_requirements.count);

    u32 family_count;
    vkGetPhysicalDeviceQueueFamilyProperties(
        info.physical_device,
        &family_count,
        0);

    TArray<VkQueueFamilyProperties> properties(&allocator);
    properties.init_range(family_count);
    DEFER(properties.release());

    for (auto& family_requirement : info.family_requirements) {
        bool found = false;
        for (int i = 0; i < family_count; ++i) {
            VkQueueFamilyProperties& family = properties[i];

            VkBool32 surface_supported = 0;
            vkGetPhysicalDeviceSurfaceSupportKHR(
                info.physical_device,
                i,
                info.surface,
                &surface_supported);

            if (family_requirement.flag_bits != 0) {
                if (!(family_requirement.flag_bits & family.queueFlags))
                    continue;
            }

            if (family_requirement.presentation != false) {
                if (!surface_supported) continue;
            }

            found = true;
            info.families.add(i);
            break;
        }

        if (!found) return Err(VK_ERROR_UNKNOWN);
    }

    std::set<u32> unique_indices;
    for (auto& family : info.families) {
        unique_indices.insert(family);
    }

    TArray<VkDeviceQueueCreateInfo> queue_create_infos(&allocator);
    queue_create_infos.init(unique_indices.size());
    float queue_priority = 1.0f;

    for (u32 family_index : unique_indices) {
        VkDeviceQueueCreateInfo queue_create_info = {
            .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = (u32)family_index,
            .queueCount       = 1,
            .pQueuePriorities = &queue_priority,
        };
        queue_create_infos.add(queue_create_info);
    }

    VkPhysicalDeviceFeatures features = {};

    VkDeviceCreateInfo create_info = {
        .sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = (u32)queue_create_infos.size,
        .pQueueCreateInfos    = queue_create_infos.data,

    };

    VkDevice result;
    VK_RETURN_IF_ERR(
        vkCreateDevice(info.physical_device, &create_info, 0, &result));

    return Ok(result);
}