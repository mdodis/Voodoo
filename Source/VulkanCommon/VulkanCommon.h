#pragma once
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>

#include "Containers/Slice.h"
#include "Core/Utility.h"
#include "Result.h"
#include "Types.h"

#define VK_RETURN_IF_ERR(x)           \
    do {                              \
        VkResult __result = (x);      \
        if (__result != VK_SUCCESS) { \
            return Err(__result);     \
        }                             \
    } while (0)

#define VK_CHECK(x)                                                    \
    do {                                                               \
        VkResult __result = (x);                                       \
        if (__result != VK_SUCCESS) {                                  \
            print(LIT("Error: {}\n"), Str(string_VkResult(__result))); \
        }                                                              \
        ASSERT(__result == VK_SUCCESS);                                \
    } while (0)

#define VK_PROC_DEBUG_CALLBACK(name)                               \
    VkBool32 VKAPI_CALL name(                                      \
        VkDebugUtilsMessageSeverityFlagBitsEXT      severity,      \
        VkDebugUtilsMessageTypeFlagsEXT             type,          \
        const VkDebugUtilsMessengerCallbackDataEXT* callback_data, \
        void*                                       user_data)

struct VertexInputInfo {
    Slice<VkVertexInputBindingDescription>   bindings;
    Slice<VkVertexInputAttributeDescription> attributes;
    VkPipelineVertexInputStateCreateFlags    flags = 0;
};

/* Helpers */

PROC_FMT_INL(VkResult) { tape->write_str(Str(string_VkResult(type))); }

bool validation_layers_exist(Allocator& allocator, Slice<const char*> layers);

struct PickPhysicalDeviceInfo {
    Slice<const char*>   device_extentions;
    VkPhysicalDeviceType prefered_device_type;
    VkSurfaceKHR         surface;
};

Result<VkPhysicalDevice, VkResult> pick_physical_device(
    Allocator& allocator, VkInstance instance, PickPhysicalDeviceInfo& info);

struct SwapChainSupportInfo {
    VkSurfaceCapabilitiesKHR   capabilities;
    TArray<VkSurfaceFormatKHR> formats;
    TArray<VkPresentModeKHR>   present_modes;

    SwapChainSupportInfo(Allocator& allocator)
        : formats(&allocator), present_modes(&allocator)
    {}

    void release()
    {
        formats.release();
        present_modes.release();
    }
};

Result<SwapChainSupportInfo, VkResult> query_physical_device_swap_chain_support(
    Allocator& allocator, VkPhysicalDevice device, VkSurfaceKHR surface);

struct CreateDeviceFamilyRequirement {
    /** Any valid flag bit for requirement, 0 for don't care */
    VkQueueFlagBits flag_bits;
    /** True for support, false for don't care */
    bool            presentation;
};

struct CreateDeviceWithQueuesInfo {
    VkSurfaceKHR                         surface;              // In
    Slice<const char*>                   validation_layers;    // In
    Slice<const char*>                   extensions;           // In
    Slice<CreateDeviceFamilyRequirement> family_requirements;  // In
    void*                                next = 0;             // In

    TArray<u32> families;  // Out
};

Result<VkDevice, VkResult> create_device_with_queues(
    Allocator&                  allocator,
    VkPhysicalDevice            device,
    CreateDeviceWithQueuesInfo& info);

struct CreateSwapchainInfo {
    VkPhysicalDevice    physical_device;  // In
    VkFormat            format;           // In
    VkColorSpaceKHR     color_space;      // In
    VkSurfaceKHR        surface;          // In
    u32                 graphics_family;  // In
    u32                 present_family;   // In
    VkExtent2D&         extent;           // In/Out
    VkSurfaceFormatKHR& surface_format;   // Out
    VkPresentModeKHR&   present_mode;     // Out
    u32&                num_images;       // Out
};

Result<VkSwapchainKHR, VkResult> create_swapchain(
    Allocator& allocator, VkDevice device, CreateSwapchainInfo& info);

TArray<VkQueueFamilyProperties> get_physical_device_queue_families(
    VkPhysicalDevice physical_device, Allocator& allocator);

void print_physical_device_queue_families(VkPhysicalDevice device);

Result<VkShaderModule, VkResult> load_shader_binary(
    Allocator& allocator, VkDevice device, Str path);

VkDescriptorSetLayoutBinding descriptor_set_layout_binding(
    VkDescriptorType type, VkShaderStageFlags stage, u32 binding);

VkWriteDescriptorSet write_descriptor_set(
    VkDescriptorType        type,
    VkDescriptorSet         set,
    VkDescriptorBufferInfo* buffer_info,
    u32                     binding);

VkWriteDescriptorSet write_descriptor_set_image(
    VkDescriptorType       type,
    VkDescriptorSet        dst_set,
    VkDescriptorImageInfo* image_info,
    u32                    binding);

VkSamplerCreateInfo make_sampler_create_info(
    VkFilter             filters,
    VkSamplerAddressMode address_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT);

VkPipelineLayoutCreateInfo make_pipeline_layout_create_info();

VkResult wait_for_fences_indefinitely(
    VkDevice       device,
    u32            fence_count,
    const VkFence* fences,
    VkBool32       wait_all = VK_TRUE,
    u64            timeout  = 1000000);