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

    VkPipelineLayout built_layout;
    TArr<VkDescriptorSetLayout, 4> set_layouts;
    TArr<u64, 4>                   set_hashes;
    TMap<Str, BindingMetadata>     bindings;

    TArray<Stage> stages;

    void add_stage(ShaderModule* mod, VkShaderStageFlagBits flags);
    // void reflect_layout(VkDevice device, )
};