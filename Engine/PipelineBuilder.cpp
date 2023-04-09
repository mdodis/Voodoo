#include "PipelineBuilder.h"

void PipelineBuilder::init_defaults()
{
    vertex_input_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = 0,
        .vertexBindingDescriptionCount   = 0,
        .vertexAttributeDescriptionCount = 0,
    };

    asm_create_info = {
        .sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext    = 0,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    rasterizer_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = 0,
        .flags = 0,
        .depthClampEnable        = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode             = VK_POLYGON_MODE_FILL,
        .cullMode                = VK_CULL_MODE_NONE,
        .frontFace               = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable         = VK_FALSE,
        .depthBiasConstantFactor = 0.f,
        .depthBiasClamp          = 0.f,
        .depthBiasSlopeFactor    = 0.f,
        .lineWidth               = 1.0f,
    };

    depth_stencil_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = 0,
        .flags = 0,
        .depthTestEnable       = VK_FALSE,
        .depthWriteEnable      = VK_FALSE,
        .depthCompareOp        = VK_COMPARE_OP_ALWAYS,
        .depthBoundsTestEnable = VK_FALSE,
        .minDepthBounds        = 0.f,
        .maxDepthBounds        = 1.f,
    };

    multisample_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = 0,
        .flags = 0,
        .rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable   = VK_FALSE,
        .minSampleShading      = 1.0f,
        .pSampleMask           = 0,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable      = VK_FALSE,
    };

    color_blend_attachment_state = {
        .blendEnable         = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp        = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp        = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };

    dynamic_state_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = 0,
        .flags = 0,
        .dynamicStateCount = 0,
        .pDynamicStates    = 0,
    };
}

PipelineBuilder& PipelineBuilder::add_shader_stage(
    VkShaderStageFlagBits stage, VkShaderModule module)
{
    VkPipelineShaderStageCreateInfo create_info = {
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage  = stage,
        .module = module,
        .pName  = "main",
    };

    shader_stages.add(create_info);
    return *this;
}

PipelineBuilder& PipelineBuilder::set_primitive_topology(
    VkPrimitiveTopology topology)
{
    asm_create_info.topology = topology;
    return *this;
}

PipelineBuilder& PipelineBuilder::set_polygon_mode(VkPolygonMode mode)
{
    rasterizer_state.polygonMode = mode;
    return *this;
}

PipelineBuilder& PipelineBuilder::set_viewport(
    float x, float y, float w, float h, float min_d, float max_d)
{
    viewport = {
        .x        = x,
        .y        = y,
        .width    = w,
        .height   = h,
        .minDepth = min_d,
        .maxDepth = max_d,
    };

    return *this;
}

PipelineBuilder& PipelineBuilder::set_scissor(i32 x, i32 y, u32 w, u32 h)
{
    scissor = {
        .offset = {x, y},
        .extent = {w, h},
    };
    return *this;
}

PipelineBuilder& PipelineBuilder::set_render_pass(VkRenderPass render_pass)
{
    this->render_pass = render_pass;
    return *this;
}

PipelineBuilder& PipelineBuilder::set_layout(VkPipelineLayout layout)
{
    this->layout = layout;
    return *this;
}

PipelineBuilder& PipelineBuilder::set_vertex_input_bindings(
    Slice<VkVertexInputBindingDescription> bindings)
{
    vertex_input_info.vertexBindingDescriptionCount = (u32)bindings.count;
    vertex_input_info.pVertexBindingDescriptions    = bindings.ptr;
    return *this;
}
PipelineBuilder& PipelineBuilder::set_vertex_input_attributes(
    Slice<VkVertexInputAttributeDescription> attributes)
{
    vertex_input_info.vertexAttributeDescriptionCount = (u32)attributes.count;
    vertex_input_info.pVertexAttributeDescriptions    = attributes.ptr;
    return *this;
}

PipelineBuilder& PipelineBuilder::set_vertex_input_info(VertexInputInfo& info)
{
    set_vertex_input_bindings(info.bindings);
    set_vertex_input_attributes(info.attributes);
    return *this;
}

PipelineBuilder& PipelineBuilder::set_depth_test(
    bool enabled, bool write, VkCompareOp compare_op)
{
    depth_stencil_state.depthTestEnable  = enabled ? VK_TRUE : VK_FALSE;
    depth_stencil_state.depthWriteEnable = write ? VK_TRUE : VK_FALSE;
    depth_stencil_state.depthCompareOp   = compare_op;
    return *this;
}

PipelineBuilder& PipelineBuilder::add_dynamic_state(
    VkDynamicState dynamic_state)
{
    dynamic_states.add(dynamic_state);
    return *this;
}

Result<VkPipeline, VkResult> PipelineBuilder::build(VkDevice device)
{
    VkPipelineViewportStateCreateInfo viewport_info = {
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports    = &viewport,
        .scissorCount  = 1,
        .pScissors     = &scissor,
    };

    VkPipelineColorBlendStateCreateInfo color_blend_state_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable   = VK_FALSE,
        .logicOp         = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments    = &color_blend_attachment_state,
    };

    VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext               = 0,
        .flags               = 0,
        .stageCount          = (u32)shader_stages.size,
        .pStages             = shader_stages.data,
        .pVertexInputState   = &vertex_input_info,
        .pInputAssemblyState = &asm_create_info,
        .pTessellationState  = 0,
        .pViewportState      = &viewport_info,
        .pRasterizationState = &rasterizer_state,
        .pMultisampleState   = &multisample_state,
        .pDepthStencilState  = &depth_stencil_state,
        .pColorBlendState    = &color_blend_state_info,
        .pDynamicState       = &dynamic_state_info,
        .layout              = layout,
        .renderPass          = render_pass,
        .subpass             = 0,
        .basePipelineHandle  = VK_NULL_HANDLE,
    };

    if (dynamic_states.size != 0) {
        dynamic_state_info.dynamicStateCount = (u32)dynamic_states.size;
        dynamic_state_info.pDynamicStates    = dynamic_states.data;
    }

    VkPipeline result;
    VK_RETURN_IF_ERR(vkCreateGraphicsPipelines(
        device,
        VK_NULL_HANDLE,
        1,
        &pipeline_info,
        0,
        &result));

    return Ok(result);
}
