#include "DescriptorBuilder.h"

DescriptorBuilder DescriptorBuilder::create(
    IAllocator&            allocator,
    DescriptorLayoutCache* cache,
    DescriptorAllocator*   descriptor_allocator)
{
    return DescriptorBuilder{
        .cache                = cache,
        .descriptor_allocator = descriptor_allocator,
        .writes               = TArray<VkWriteDescriptorSet>(&allocator),
        .bindings = TArray<VkDescriptorSetLayoutBinding>(&allocator),
    };
}

DescriptorBuilder& DescriptorBuilder::bind_buffer(
    u32                     binding,
    VkDescriptorBufferInfo* buffer_info,
    VkDescriptorType        type,
    VkShaderStageFlags      stage_flags)
{
    VkDescriptorSetLayoutBinding new_binding = {
        .binding            = binding,
        .descriptorType     = type,
        .descriptorCount    = 1,
        .stageFlags         = stage_flags,
        .pImmutableSamplers = nullptr,
    };
    bindings.add(new_binding);

    VkWriteDescriptorSet new_write = {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext           = 0,
        .dstBinding      = binding,
        .descriptorCount = 1,
        .descriptorType  = type,
        .pBufferInfo     = buffer_info,
    };
    writes.add(new_write);

    return *this;
}

DescriptorBuilder& DescriptorBuilder::bind_image(
    u32                    binding,
    VkDescriptorImageInfo* image_info,
    VkDescriptorType       type,
    VkShaderStageFlags     stage_flags)
{
    VkDescriptorSetLayoutBinding new_binding = {
        .binding            = binding,
        .descriptorType     = type,
        .descriptorCount    = 1,
        .stageFlags         = stage_flags,
        .pImmutableSamplers = nullptr,
    };
    bindings.add(new_binding);

    VkWriteDescriptorSet new_write = {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext           = 0,
        .dstBinding      = binding,
        .descriptorCount = 1,
        .descriptorType  = type,
        .pImageInfo      = image_info,
    };
    writes.add(new_write);

    return *this;
}

bool DescriptorBuilder::build(
    VkDescriptorSet& set, VkDescriptorSetLayout& layout)
{
    VkDescriptorSetLayoutCreateInfo create_info = {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext        = 0,
        .flags        = 0,
        .bindingCount = (u32)bindings.size,
        .pBindings    = bindings.data,
    };

    layout = cache->create_layout(&create_info);

    bool success = descriptor_allocator->allocate(&set, layout);
    if (!success) return false;

    for (VkWriteDescriptorSet& w : writes) {
        w.dstSet = set;
    }

    vkUpdateDescriptorSets(
        descriptor_allocator->device,
        (u32)writes.size,
        writes.data,
        0,
        nullptr);
}
