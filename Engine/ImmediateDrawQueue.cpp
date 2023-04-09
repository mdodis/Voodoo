#include "ImmediateDrawQueue.h"

#include "Containers/Extras.h"
#include "PipelineBuilder.h"

static VkShaderModule load_shader(
    IAllocator& allocator, Str path, VkDevice device)
{
    CREATE_SCOPED_ARENA(&allocator, temp_alloc, KILOBYTES(8));
    auto load_result = load_shader_binary(temp_alloc, device, path);

    if (!load_result.ok()) {
        print(LIT("Failed to load & create shader module {}\n"), path);
        return VK_NULL_HANDLE;
    }

    print(LIT("Loaded shader module binary {}\n"), path);
    return load_result.value();
}

void ImmediateDrawQueue::init(
    VkDevice                     device,
    VkRenderPass                 render_pass,
    Slice<VkDescriptorSetLayout> descriptors)
{
    CREATE_SCOPED_ARENA(&System_Allocator, temp, KILOBYTES(1));

    VkPipelineLayoutCreateInfo create_info = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount         = (u32)descriptors.count,
        .pSetLayouts            = descriptors.ptr,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges    = 0,
    };

    VK_CHECK(vkCreatePipelineLayout(device, &create_info, 0, &pipeline_layout));

    VertexInputInfo vertex_input_info = {
        .bindings   = alloc_slice<VkVertexInputBindingDescription>(temp, 1),
        .attributes = alloc_slice<VkVertexInputAttributeDescription>(temp, 1),
    };

    vertex_input_info.bindings[0] = {
        .binding   = 0,
        .stride    = sizeof(ImmediateDrawQueue::Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };

    vertex_input_info.attributes[0] = {
        .location = 0,
        .binding  = 0,
        .format   = VK_FORMAT_R32G32B32_SFLOAT,
        .offset   = OFFSET_OF(ImmediateDrawQueue::Vertex, position),
    };

    PipelineBuilder(temp)
        .add_dynamic_state(VK_DYNAMIC_STATE_VIEWPORT)
        .add_dynamic_state(VK_DYNAMIC_STATE_SCISSOR)
        .set_primitive_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .set_polygon_mode(VK_POLYGON_MODE_FILL)
        .set_render_pass(render_pass)
        .set_depth_test(false, true, VK_COMPARE_OP_ALWAYS)
        .set_vertex_input_info(vertex_input_info)
        .build(device)
        .unwrap();
}