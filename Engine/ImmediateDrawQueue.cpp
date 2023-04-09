#include "ImmediateDrawQueue.h"

#include "PipelineBuilder.h"

void ImmediateDrawQueue::init(VkDevice device, VkRenderPass render_pass)
{
    CREATE_SCOPED_ARENA(&System_Allocator, temp, KILOBYTES(1));

    PipelineBuilder(temp)
        .add_dynamic_state(VK_DYNAMIC_STATE_VIEWPORT)
        .add_dynamic_state(VK_DYNAMIC_STATE_SCISSOR)
        .set_primitive_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .set_polygon_mode(VK_POLYGON_MODE_FILL)
        .set_render_pass(render_pass)
        .set_depth_test(false, true, VK_COMPARE_OP_ALWAYS)
        .build(device)
        .unwrap();
}