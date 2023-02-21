#pragma once
#include "Window.h"
#include <vulkan/vulkan.h>
#include "Result.h"
#include "Containers/Array.h"
#include "Serialization/Base.h"
#include "Reflection.h"
#include "Str.h"

struct AppConfig {
    bool validation_layers = true;
    bool use_discrete_gpu = true;
};

struct QueueFamilyIndices {
    int graphics;
};

struct App {
    Window window;
    AppConfig config;
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;

    Result<void, VkResult> init_vulkan();
    void deinit();

private:
    bool check_validation_layers(IAllocator& allocator);
    bool is_physical_device_suitable(VkPhysicalDevice& device);
    Result<QueueFamilyIndices, VkResult> find_queue_family_indices(IAllocator& allocator, VkPhysicalDevice device);
};

struct AppConfigDescriptor : IDescriptor {
    PrimitiveDescriptor<bool> desc_validation_layers = { OFFSET_OF(AppConfig, validation_layers), LIT("validationLayers") };
    PrimitiveDescriptor<bool> desc_use_discrete_gpu  = { OFFSET_OF(AppConfig, use_discrete_gpu),  LIT("useDiscreteGPU") };

    IDescriptor* descs[2] = {
        &desc_validation_layers,
        &desc_use_discrete_gpu
    };

    CUSTOM_DESC_DEFAULT(AppConfigDescriptor)

    // Inherited via IDescriptor
    virtual Str type_name() override { return LIT("AppConfig"); };

    virtual Slice<IDescriptor*> subdescriptors() override {
        return Slice<IDescriptor*>(descs, ARRAY_COUNT(descs));
    };

};

DEFINE_DESCRIPTOR_OF_INL(AppConfig);