#include "DescriptorLayoutCache.h"

#include "Sort.h"

void DescriptorLayoutCache::init(VkDevice device) { this->device = device; }

VkDescriptorSetLayout DescriptorLayoutCache::create_layout(
    VkDescriptorSetLayoutCreateInfo* info)
{
    DescriptorLayoutCache::DescriptorLayoutInfo layout_info(allocator);
    int                                         last_binding = -1;
    bool                                        is_sorted    = true;

    for (u32 i = 0; i < info->bindingCount; ++i) {
        if (info->pBindings[i].binding > last_binding) {
            last_binding = info->pBindings[i].binding;

        } else {
            is_sorted = false;
        }

        layout_info.bindings.add(info->pBindings[i]);
    }

    // Sort if not sorted
    if (!is_sorted) {
        auto s = slice(layout_info.bindings);
        sort::quicksort(s);
    }

    if (layout_cache.contains(layout_info)) {
        auto result = layout_cache[layout_info];
        layout_info.release();
        return result;
    }

    VkDescriptorSetLayout layout;
    vkCreateDescriptorSetLayout(device, info, 0, &layout);
    layout_cache.add(layout_info, layout);
    return layout;
}

void DescriptorLayoutCache::deinit()
{
    for (auto pair : layout_cache) {
        pair.key.release();
        vkDestroyDescriptorSetLayout(device, pair.val, nullptr);
    }

    layout_cache.release();
}