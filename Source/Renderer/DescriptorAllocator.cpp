#include "DescriptorAllocator.h"

Result<VkDescriptorPool, VkResult> create_pool(
    IAllocator&                          allocator,
    VkDevice                             device,
    Slice<DescriptorAllocator::PoolSize> pool_sizes,
    int                                  count,
    VkDescriptorPoolCreateFlags          flags)
{
    CREATE_SCOPED_ARENA(&allocator, temp, KILOBYTES(1));

    TArray<VkDescriptorPoolSize> sizes(&temp);

    for (const auto& sz : pool_sizes) {
        sizes.add(VkDescriptorPoolSize{
            .type            = sz.type,
            .descriptorCount = u32(sz.t * count),
        });
    }

    VkDescriptorPoolCreateInfo create_info = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext         = 0,
        .flags         = flags,
        .maxSets       = (u32)count,
        .poolSizeCount = (u32)sizes.size,
        .pPoolSizes    = sizes.data,
    };

    VkDescriptorPool result;
    VK_RETURN_IF_ERR(
        vkCreateDescriptorPool(device, &create_info, nullptr, &result));

    return Ok(result);
}

void DescriptorAllocator::init(IAllocator& allocator, VkDevice device)
{
    used_pools.alloc = &allocator;
    free_pools.alloc = &allocator;
    this->device     = device;
}

VkDescriptorPool DescriptorAllocator::grab_pool()
{
    if (free_pools.size > 0) {
        VkDescriptorPool result = *free_pools.last();
        free_pools.pop();
        return result;
    } else {
        return create_pool(
                   *free_pools.alloc,
                   device,
                   slice(pool_sizes, ARRAY_COUNT(pool_sizes)),
                   1000,
                   0)
            .unwrap();
    }
}

bool DescriptorAllocator::allocate(
    VkDescriptorSet* set, VkDescriptorSetLayout layout)
{
    if (current_pool == VK_NULL_HANDLE) {
        current_pool = grab_pool();
        used_pools.add(current_pool);
    }

    VkDescriptorSetAllocateInfo alloc_info = {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext              = 0,
        .descriptorPool     = current_pool,
        .descriptorSetCount = 1,
        .pSetLayouts        = &layout,
    };

    VkResult result = vkAllocateDescriptorSets(device, &alloc_info, set);
    bool     need_reallocate = false;
    switch (result) {
        case VK_SUCCESS: {
            return true;
        } break;

        case VK_ERROR_FRAGMENTED_POOL:
        case VK_ERROR_OUT_OF_POOL_MEMORY: {
            need_reallocate = true;
        } break;

        default: {
            return false;

        } break;
    }

    if (need_reallocate) {
        current_pool = grab_pool();
        used_pools.add(current_pool);
        alloc_info.descriptorPool = current_pool;
        result = vkAllocateDescriptorSets(device, &alloc_info, set);

        if (result == VK_SUCCESS) return true;
    }

    return false;
}

void DescriptorAllocator::reset_pools()
{
    for (auto p : used_pools) {
        vkResetDescriptorPool(device, p, 0);
        free_pools.add(p);
    }

    used_pools.empty();

    current_pool = VK_NULL_HANDLE;
}

void DescriptorAllocator::deinit()
{
    for (auto pool : used_pools) {
        vkDestroyDescriptorPool(device, pool, nullptr);
    }

    for (auto pool : free_pools) {
        vkDestroyDescriptorPool(device, pool, nullptr);
    }
}
