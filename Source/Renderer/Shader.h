#pragma once
#include "Containers/Map.h"
#include "Containers/Slice.h"
#include "DescriptorAllocator.h"
#include "DescriptorBuilder.h"
#include "RendererTypes.h"

struct ShaderModule {
    Slice<u8>      code;
    VkShaderModule mod;

    static Result<ShaderModule, VkResult> load(VkDevice device, Str path);
};

u64 hash_of(VkDescriptorSetLayoutCreateInfo& info, u32 seed);

struct ShaderEffect {
    struct BindingMetadata {
        u32              set;
        u32              binding;
        VkDescriptorType type;
    };

    struct Stage {
        ShaderModule*         mod;
        VkShaderStageFlagBits stage;
    };

    struct MetadataOverride {
        Str              name;
        VkDescriptorType overriden_type;
    };

    void init(Allocator& allocator);

    VkPipelineLayout               built_layout;
    TArr<VkDescriptorSetLayout, 4> set_layouts;
    TArr<u64, 4>                   set_hashes;
    TMap<Str, BindingMetadata>     bindings;

    TArray<Stage> stages;

    void add_stage(ShaderModule* mod, VkShaderStageFlagBits flags);
    void reflect_layout(
        VkDevice device, const Slice<MetadataOverride> overrides);

    void fill_stages(TArray<VkPipelineShaderStageCreateInfo>& pipeline_stages);

    u32 num_valid_layouts() const
    {
        u32 result = 0;
        for (const VkDescriptorSetLayout& set_layout : set_layouts) {
            if (set_layout != VK_NULL_HANDLE) result++;
        }
        return result;
    }
};

struct ShaderCache {
    void          init(Allocator& allocator, VkDevice device);
    ShaderModule* get_shader(Str path);
    void          deinit();

    VkDevice                device;
    TMap<Str, ShaderModule> module_cache;
};