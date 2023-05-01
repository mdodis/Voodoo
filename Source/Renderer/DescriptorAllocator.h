#pragma once
#include "Containers/Array.h"
#include "VulkanCommon/VulkanCommon.h"

struct DescriptorAllocator {
    struct PoolSize {
        VkDescriptorType type;
        float            t;
    };

    void             init(Allocator& allocator, VkDevice device);
    void             reset_pools();
    VkDescriptorPool grab_pool();
    bool allocate(VkDescriptorSet* set, VkDescriptorSetLayout layout);
    void deinit();

    PoolSize pool_sizes[11] = {
        {VK_DESCRIPTOR_TYPE_SAMPLER, 0.5f},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4.f},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.f},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1.f},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1.f},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2.f},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2.f},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0.5f},
    };

    VkDevice         device       = VK_NULL_HANDLE;
    VkDescriptorPool current_pool = VK_NULL_HANDLE;

    TArray<VkDescriptorPool> used_pools;
    TArray<VkDescriptorPool> free_pools;
};
