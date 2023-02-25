#pragma once
#include <vulkan/vulkan.h>

#include "Containers/Array.h"
#include "Reflection.h"
#include "Result.h"
#include "Serialization/Base.h"
#include "Str.h"
#include "Window.h"

struct AppConfig {
    bool validation_layers = true;
    bool use_discrete_gpu  = true;
};

struct QueueFamilyIndices {
    int graphics;
    int present;
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR   capabilities;
    TArray<VkSurfaceFormatKHR> formats;
    TArray<VkPresentModeKHR>   present_modes;

    SwapChainSupportDetails(IAllocator& allocator)
        : formats(&allocator), present_modes(&allocator)
    {}

    void release()
    {
        formats.release();
        present_modes.release();
    }
};

struct Shader {
    VkShaderModule        module;
    VkShaderStageFlagBits stage;
};

struct App {
    Window                window;
    AppConfig             config;
    VkInstance            instance        = VK_NULL_HANDLE;
    VkPhysicalDevice      physical_device = VK_NULL_HANDLE;
    VkDevice              device          = VK_NULL_HANDLE;
    VkQueue               graphics_queue  = VK_NULL_HANDLE;
    VkQueue               present_queue   = VK_NULL_HANDLE;
    VkSurfaceKHR          surface         = VK_NULL_HANDLE;
    VkSwapchainKHR        swap_chain      = VK_NULL_HANDLE;
    VkSurfaceFormatKHR    surface_format;
    VkPresentModeKHR      present_mode;
    VkExtent2D            extent;
    TArray<VkImage>       swap_chain_images;
    TArray<VkImageView>   swap_chain_views;
    TArray<VkFramebuffer> framebuffers;

    VkRenderPass     render_pass;
    VkPipelineLayout pipeline_layout;
    VkPipeline       graphics_pipeline;
    VkCommandPool    cmd_pool;
    VkCommandBuffer  cmd_buffer;

    Shader sh_vertex;
    Shader sh_fragment;

    VkSemaphore sem_image_available;
    VkSemaphore sem_render_finished;
    VkFence     fence_in_flight;

    Result<void, VkResult>            init_vulkan();
    void                              init_pipeline();
    Result<VkCommandBuffer, VkResult> create_cmd_buffer(VkCommandPool pool);
    void                              recompile_shaders();
    void                              deinit();
    void record_cmd_buffer(VkCommandBuffer buffer, u32 image_idx);

private:
    bool check_validation_layers(IAllocator& allocator);
    bool is_physical_device_suitable(
        VkPhysicalDevice& device, IAllocator& allocator);
    Result<QueueFamilyIndices, VkResult> find_queue_family_indices(
        IAllocator& allocator, VkPhysicalDevice device);
    SwapChainSupportDetails query_swap_chain_support(
        IAllocator& allocator, VkPhysicalDevice qdevice);
    Shader load_shader(Str path, VkShaderStageFlagBits stage);
};

struct AppConfigDescriptor : IDescriptor {
    PrimitiveDescriptor<bool> desc_validation_layers = {
        OFFSET_OF(AppConfig, validation_layers), LIT("validationLayers")};
    PrimitiveDescriptor<bool> desc_use_discrete_gpu = {
        OFFSET_OF(AppConfig, use_discrete_gpu), LIT("useDiscreteGPU")};

    IDescriptor* descs[2] = {&desc_validation_layers, &desc_use_discrete_gpu};

    CUSTOM_DESC_DEFAULT(AppConfigDescriptor)

    // Inherited via IDescriptor
    virtual Str type_name() override { return LIT("AppConfig"); };

    virtual Slice<IDescriptor*> subdescriptors() override
    {
        return Slice<IDescriptor*>(descs, ARRAY_COUNT(descs));
    };
};

DEFINE_DESCRIPTOR_OF_INL(AppConfig);