#pragma once
#include "Containers/Map.h"
#include "PipelineBuilder.h"
#include "Shader.h"

struct MaterialTexture {
    Str slot;
    Str texture;
};

struct MaterialAsset {
    Str                     base_effect;
    TArray<MaterialTexture> textures;
};

struct ShaderPass {
    ShaderEffect*    effect   = nullptr;
    VkPipeline       pipeline = VK_NULL_HANDLE;
    VkPipelineLayout layout   = VK_NULL_HANDLE;
};

struct SampledTexture {
    VkSampler   sampler;
    VkImageView view;

    _inline bool operator==(const SampledTexture& other) const
    {
        return (sampler == other.sampler) && (view == other.view);
    }
};

struct ShaderParameters {};

struct EffectTemplate {
    ShaderPass*       pass_shaders;
    ShaderParameters* parameters;
};

struct MaterialInstance {
    EffectTemplate* base;
    VkDescriptorSet pass_sets;

    Slice<SampledTexture> textures;
    ShaderParameters*     parameters;
};

struct MaterialData {
    Slice<SampledTexture> textures;
    Str                   base_template;

    bool operator==(const MaterialData& other) const;
};

_inline bool hash_of(const MaterialData& data, u32 seed)
{
    u64 result = hash_of(data.base_template, seed);

    for (const SampledTexture& texture : data.textures) {
        u64 sampler_handle = (u64)texture.sampler;
        u64 view_handle    = (u64)texture.view;

        result ^= hash_of(
            (hash_of(sampler_handle, seed) << 3) ^
                (hash_of(view_handle, seed) >> 7),
            seed);
    }

    return result;
}

struct MaterialSystem {
    void init(struct Renderer* renderer);
    void deinit();

    ShaderEffect* build_effect(Str vert_path, Str frag_path);
    ShaderPass*   build_shader(
          VkRenderPass     render_pass,
          PipelineBuilder& builder,
          ShaderEffect*    effect);

    MaterialInstance* build_material(
        Str material_name, const MaterialData& material_data);

private:
    TMap<Str, EffectTemplate>             template_cache;
    TMap<Str, MaterialInstance*>          materials;
    TMap<MaterialData, MaterialInstance*> material_cache;
    PipelineBuilder                       forward_builder;
    struct Renderer*                      owner;
};