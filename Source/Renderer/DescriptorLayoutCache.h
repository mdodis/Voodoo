#pragma once
#include "Containers/Map.h"
#include "Hashing.h"
#include "VulkanCommon/VulkanCommon.h"

struct DescriptorLayoutCache {
    DescriptorLayoutCache(IAllocator& allocator = System_Allocator)
        : layout_cache(allocator), allocator(allocator)
    {}

    struct DescriptorLayoutInfo {
        DescriptorLayoutInfo(IAllocator& allocator) : bindings(&allocator) {}

        TArray<VkDescriptorSetLayoutBinding> bindings;
        void                                 release() { bindings.release(); }
    };

    void                  init(VkDevice device);
    VkDescriptorSetLayout create_layout(VkDescriptorSetLayoutCreateInfo* info);
    void                  deinit();

    IAllocator&                                       allocator;
    TMap<DescriptorLayoutInfo, VkDescriptorSetLayout> layout_cache;
    VkDevice                                          device;
};

static _inline u64 hash_of(
    const DescriptorLayoutCache::DescriptorLayoutInfo& info, u32 seed)
{
    u64 result = hash_of(info.bindings.size, seed);

    for (const auto& binding : info.bindings) {
        u64 packed =
            (binding.binding) | (binding.descriptorType << 8) |
            (binding.descriptorCount << 16) | (binding.stageFlags << 24);

        char* parr = (char*)&packed;
        u64   hash = murmur_hash2(parr, sizeof(packed), seed);

        result ^= hash;
    }

    return result;
}

static _inline bool operator==(
    const DescriptorLayoutCache::DescriptorLayoutInfo& left,
    const DescriptorLayoutCache::DescriptorLayoutInfo& right)
{
    if (left.bindings.size != right.bindings.size) return false;

    for (u64 i = 0; i < left.bindings.size; ++i) {
        const VkDescriptorSetLayoutBinding& lb = left.bindings[i];
        const VkDescriptorSetLayoutBinding& rb = right.bindings[i];

        if (lb.binding != rb.binding) return false;
        if (lb.descriptorType != rb.descriptorType) return false;
        if (lb.descriptorCount != rb.descriptorCount) return false;
        if (lb.stageFlags != rb.stageFlags) return false;
        if (lb.pImmutableSamplers != rb.pImmutableSamplers) return false;
    }

    return true;
}

static _inline bool operator!=(
    const DescriptorLayoutCache::DescriptorLayoutInfo& left,
    const DescriptorLayoutCache::DescriptorLayoutInfo& right)
{
    return !(left == right);
}

static _inline bool operator<(
    const VkDescriptorSetLayoutBinding& left,
    const VkDescriptorSetLayoutBinding& right)
{
    return left.binding < right.binding;
}