#pragma once
#include "Containers/Array.h"
#include "DescriptorAllocator.h"
#include "DescriptorLayoutCache.h"

struct DescriptorBuilder {
    static DescriptorBuilder create(
        IAllocator&            allocator,
        DescriptorLayoutCache* cache,
        DescriptorAllocator*   descriptor_allocator);

    DescriptorBuilder& bind_buffer(
        u32                     binding,
        VkDescriptorBufferInfo* buffer_info,
        VkDescriptorType        type,
        VkShaderStageFlags      stage_flags);

    DescriptorBuilder& bind_image(
        u32                    binding,
        VkDescriptorImageInfo* image_info,
        VkDescriptorType       type,
        VkShaderStageFlags     stage_flags);

    bool build(VkDescriptorSet& set, VkDescriptorSetLayout& layout);
    bool build(VkDescriptorSet& set)
    {
        VkDescriptorSetLayout layout;
        return build(set, layout);
    }

    DescriptorLayoutCache*               cache;
    DescriptorAllocator*                 descriptor_allocator;
    TArray<VkWriteDescriptorSet>         writes;
    TArray<VkDescriptorSetLayoutBinding> bindings;
};