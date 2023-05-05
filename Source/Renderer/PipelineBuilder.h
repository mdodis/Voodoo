#pragma once
#include "VulkanCommon/VulkanCommon.h"

struct PipelineBuilder {
    PipelineBuilder() = default;
    PipelineBuilder(Allocator& allocator)
        : shader_stages(&allocator), dynamic_states(&allocator)
    {
        init_defaults();
    }

    TArray<VkPipelineShaderStageCreateInfo> shader_stages;
    TArray<VkDynamicState>                  dynamic_states;
    VkPipelineVertexInputStateCreateInfo    vertex_input_info;
    VkPipelineInputAssemblyStateCreateInfo  asm_create_info;
    VkViewport                              viewport;
    VkRect2D                                scissor;
    VkPipelineRasterizationStateCreateInfo  rasterizer_state;
    VkPipelineDepthStencilStateCreateInfo   depth_stencil_state;
    VkPipelineMultisampleStateCreateInfo    multisample_state;
    VkPipelineColorBlendAttachmentState     color_blend_attachment_state;
    VkPipelineDynamicStateCreateInfo        dynamic_state_info;
    VkPipelineLayout                        layout;
    VkRenderPass                            render_pass;

    bool has_depth_test  = false;
    bool has_depth_write = false;

    PipelineBuilder& add_shader_stage(
        VkShaderStageFlagBits stage, VkShaderModule module);
    PipelineBuilder& set_effect(struct ShaderEffect* effect);
    PipelineBuilder& set_primitive_topology(VkPrimitiveTopology topology);
    PipelineBuilder& set_polygon_mode(VkPolygonMode mode);
    PipelineBuilder& set_viewport(
        float x, float y, float w, float h, float min_d, float max_d);
    PipelineBuilder& set_scissor(i32 x, i32 y, u32 w, u32 h);
    PipelineBuilder& set_render_pass(VkRenderPass render_pass);
    PipelineBuilder& set_layout(VkPipelineLayout layout);
    PipelineBuilder& set_vertex_input_bindings(
        Slice<VkVertexInputBindingDescription> bindings);
    PipelineBuilder& set_vertex_input_attributes(
        Slice<VkVertexInputAttributeDescription> attributes);
    PipelineBuilder& set_vertex_input_info(VertexInputInfo& info);
    PipelineBuilder& set_depth_test(
        bool enabled, bool write, VkCompareOp compare_op);
    PipelineBuilder& set_cull_mode(VkCullModeFlags mode);
    PipelineBuilder& add_dynamic_state(VkDynamicState dynamic_state);

    void                         init_defaults();
    Result<VkPipeline, VkResult> build(VkDevice device);
};
