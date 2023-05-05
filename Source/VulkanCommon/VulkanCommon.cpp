#define MOK_WIN32_NO_FUNCTIONS
#include "VulkanCommon.h"

#include <set>

#include "Containers/Extras.h"

bool validation_layers_exist(Allocator& allocator, Slice<const char*> layers)
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
    Allocator& allocator, VkInstance instance, PickPhysicalDeviceInfo& info)
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
    for (u32 i = 0; i < device_count; ++i) {
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

        // @todo: cant query willy nilly
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
    Allocator& allocator, VkPhysicalDevice device, VkSurfaceKHR surface)
{
    SwapChainSupportInfo result(allocator);

    bool has_queue_with_present = false;
    auto families = get_physical_device_queue_families(device, allocator);
    DEFER(families.release());
    for (u32 i = 0; i < families.size; ++i) {
        VkQueueFamilyProperties& family            = families[i];
        VkBool32                 surface_supported = false;
        VK_RETURN_IF_ERR(vkGetPhysicalDeviceSurfaceSupportKHR(
            device,
            i,
            surface,
            &surface_supported));

        if (surface_supported) {
            has_queue_with_present = true;
            break;
        }
    }

    if (!has_queue_with_present) {
        return Ok(result);
    }

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
    Allocator&                  allocator,
    VkPhysicalDevice            device,
    CreateDeviceWithQueuesInfo& info)
{
    info.families.alloc = &allocator;
    info.families.init(info.family_requirements.count);

    u32 family_count;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, 0);

    TArray<VkQueueFamilyProperties> properties(&allocator);
    properties.init_range(family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(
        device,
        &family_count,
        properties.data);
    DEFER(properties.release());

    for (auto& family_requirement : info.family_requirements) {
        bool found = false;
        for (u32 i = 0; i < family_count; ++i) {
            VkQueueFamilyProperties& family = properties[i];

            VkBool32 surface_supported = 0;
            vkGetPhysicalDeviceSurfaceSupportKHR(
                device,
                i,
                info.surface,
                &surface_supported);

            if (family_requirement.flag_bits != 0) {
                if (!(family.queueFlags & family_requirement.flag_bits))
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
    // @todo: Move this out of here!
    features.multiDrawIndirect        = VK_TRUE;
    features.fillModeNonSolid         = VK_TRUE;

    VkDeviceCreateInfo create_info = {
        .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext                   = info.next,
        .queueCreateInfoCount    = (u32)queue_create_infos.size,
        .pQueueCreateInfos       = queue_create_infos.data,
        .enabledLayerCount       = (u32)info.validation_layers.count,
        .ppEnabledLayerNames     = info.validation_layers.ptr,
        .enabledExtensionCount   = (u32)info.extensions.count,
        .ppEnabledExtensionNames = info.extensions.ptr,
        .pEnabledFeatures        = &features,
    };

    VkDevice result;
    VK_RETURN_IF_ERR(vkCreateDevice(device, &create_info, 0, &result));

    return Ok(result);
}

Result<VkSwapchainKHR, VkResult> create_swapchain(
    Allocator& allocator, VkDevice device, CreateSwapchainInfo& info)
{
    auto swap_chain_support_result = query_physical_device_swap_chain_support(
        allocator,
        info.physical_device,
        info.surface);

    if (!swap_chain_support_result.ok())
        return Err(swap_chain_support_result.err());

    SwapChainSupportInfo support = swap_chain_support_result.value();

    info.surface_format = support.formats[0];
    for (const auto& format : support.formats) {
        if (format.format == info.format &&
            format.colorSpace == info.color_space)
        {
            info.surface_format = format;
            break;
        }
    }

    // Present mode
    info.present_mode = VK_PRESENT_MODE_FIFO_KHR;
    // for (const VkPresentModeKHR& mode : details.present_modes) {
    //     if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
    //         info.present_mode = mode;
    //         break;
    //     }
    // }

    // Choose extents
    if (support.capabilities.currentExtent.width != NumProps<u32>::max) {
        info.extent = support.capabilities.currentExtent;
    } else {
        info.extent.width = clamp(
            info.extent.width,
            support.capabilities.minImageExtent.width,
            support.capabilities.maxImageExtent.width);

        info.extent.height = clamp(
            info.extent.height,
            support.capabilities.minImageExtent.height,
            support.capabilities.maxImageExtent.height);
    }

    info.num_images = support.capabilities.minImageCount + 1;
    if ((support.capabilities.maxImageCount > 0) &&
        (info.num_images > support.capabilities.maxImageCount))
    {
        info.num_images = support.capabilities.maxImageCount;
    }

    // Create swap chain
    VkSwapchainCreateInfoKHR create_info = {
        .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface          = info.surface,
        .minImageCount    = info.num_images,
        .imageFormat      = info.surface_format.format,
        .imageColorSpace  = info.surface_format.colorSpace,
        .imageExtent      = info.extent,
        .imageArrayLayers = 1,
        .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform     = support.capabilities.currentTransform,
        .compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode      = info.present_mode,
        .clipped          = VK_TRUE,
        .oldSwapchain     = VK_NULL_HANDLE,
    };

    u32 queue_family_indices[] = {
        info.graphics_family,
        info.present_family,
    };

    if (info.graphics_family != info.present_family) {
        create_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices   = queue_family_indices;
    } else {
        create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices   = 0;
    }

    VkSwapchainKHR swapchain;
    VK_RETURN_IF_ERR(vkCreateSwapchainKHR(device, &create_info, 0, &swapchain));

    return Ok(swapchain);
}

void print_physical_device_queue_families(VkPhysicalDevice device)
{
    u32 family_count;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, 0);

    TArray<VkQueueFamilyProperties> properties(&System_Allocator);
    properties.init_range(family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(
        device,
        &family_count,
        properties.data);
    DEFER(properties.release());

    for (u32 i = 0; i < properties.size; ++i) {
        VkQueueFamilyProperties& family = properties[i];
        print(LIT("Queue Family: {} ({} queues)\n"), i, family.queueCount);
        print(
            LIT("\tGraphics: {}\n"),
            family.queueFlags & VK_QUEUE_GRAPHICS_BIT);
    }
}

Result<VkShaderModule, VkResult> load_shader_binary(
    Allocator& allocator, VkDevice device, Str path)
{
    Raw data = dump_file(path, allocator);
    DEFER(allocator.release(data.buffer));

    VkShaderModuleCreateInfo create_info = {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = data.size,
        .pCode    = (u32*)data.buffer,
    };

    VkShaderModule result;
    VK_RETURN_IF_ERR(vkCreateShaderModule(device, &create_info, 0, &result));

    return Ok(result);
}

VkDescriptorSetLayoutBinding descriptor_set_layout_binding(
    VkDescriptorType type, VkShaderStageFlags stage, u32 binding)
{
    return VkDescriptorSetLayoutBinding{
        .binding            = binding,
        .descriptorType     = type,
        .descriptorCount    = 1,
        .stageFlags         = stage,
        .pImmutableSamplers = 0,
    };
}

VkWriteDescriptorSet write_descriptor_set(
    VkDescriptorType        type,
    VkDescriptorSet         set,
    VkDescriptorBufferInfo* buffer_info,
    u32                     binding)
{
    return VkWriteDescriptorSet{
        .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext            = 0,
        .dstSet           = set,
        .dstBinding       = binding,
        .dstArrayElement  = 0,
        .descriptorCount  = 1,
        .descriptorType   = type,
        .pImageInfo       = 0,
        .pBufferInfo      = buffer_info,
        .pTexelBufferView = 0,
    };
}

VkWriteDescriptorSet write_descriptor_set_image(
    VkDescriptorType       type,
    VkDescriptorSet        dst_set,
    VkDescriptorImageInfo* image_info,
    u32                    binding)
{
    return VkWriteDescriptorSet{
        .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext            = 0,
        .dstSet           = dst_set,
        .dstBinding       = binding,
        .dstArrayElement  = 0,
        .descriptorCount  = 1,
        .descriptorType   = type,
        .pImageInfo       = image_info,
        .pBufferInfo      = 0,
        .pTexelBufferView = 0,
    };
}

VkSamplerCreateInfo make_sampler_create_info(
    VkFilter filters, VkSamplerAddressMode address_mode)
{
    return VkSamplerCreateInfo{
        .sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext        = 0,
        .magFilter    = filters,
        .minFilter    = filters,
        .addressModeU = address_mode,
        .addressModeV = address_mode,
        .addressModeW = address_mode,
    };
}

TArray<VkQueueFamilyProperties> get_physical_device_queue_families(
    VkPhysicalDevice physical_device, Allocator& allocator)
{
    u32 count;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, 0);

    TArray<VkQueueFamilyProperties> result(&allocator);
    result.init_range(count);

    vkGetPhysicalDeviceQueueFamilyProperties(
        physical_device,
        &count,
        result.data);

    return result;
}

VkResult wait_for_fences_indefinitely(
    VkDevice       device,
    u32            fence_count,
    const VkFence* fences,
    VkBool32       wait_all,
    u64            timeout)
{
    VkResult result = VK_SUCCESS;

    for (;;) {
        result =
            vkWaitForFences(device, fence_count, fences, wait_all, timeout);
        if (result == VK_SUCCESS) {
            break;
        } else if (result == VK_TIMEOUT) {
            continue;
        } else {
            return result;
        }
    }
    return VK_SUCCESS;
}

VkPipelineLayoutCreateInfo make_pipeline_layout_create_info()
{
    return VkPipelineLayoutCreateInfo{
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext                  = nullptr,
        .flags                  = 0,
        .setLayoutCount         = 0,
        .pSetLayouts            = nullptr,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges    = nullptr,
    };
}

VkPipelineShaderStageCreateInfo make_pipeline_shader_stage_create_info(
    VkShaderStageFlagBits stage, VkShaderModule module)
{
    return VkPipelineShaderStageCreateInfo{
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext  = nullptr,
        .stage  = stage,
        .module = module,
        .pName  = "main",
    };
}