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

struct MaterialSystem {
    void init(struct Renderer* renderer);
    void deinit();

    PipelineBuilder  forward_builder;
    struct Renderer* owner;

private:
    ShaderEffect* build_effect(Str vert_path, Str frag_path);
    ShaderPass*   build_shader(
          VkRenderPass     render_pass,
          PipelineBuilder& builder,
          ShaderEffect*    effect);
};