#include "ImmediateDrawQueue.h"

#include <glm/gtc/matrix_transform.hpp>

#include "Containers/Extras.h"
#include "DescriptorBuilder.h"
#include "PipelineBuilder.h"

// clang-format off
static ImmediateDrawQueue::Vertex Cube_Vertices[] = {
    {{-0.500000f, -0.500000f, 0.500000f}},
    {{-0.500000f, 0.500000f, 0.500000f}},
    {{-0.500000f, 0.500000f, -0.500000f}},
    {{-0.500000f, -0.500000f, 0.500000f}},
    {{-0.500000f, 0.500000f, -0.500000f}},
    {{-0.500000f, -0.500000f, -0.500000f}},
    {{-0.500000f, -0.500000f, -0.500000f}},
    {{-0.500000f, 0.500000f, -0.500000f}},
    {{0.500000f,  0.500000f, -0.500000f}},
    {{-0.500000f, -0.500000f, -0.500000f}},
    {{0.500000f,  0.500000f, -0.500000f}},
    {{0.500000f,  -0.500000f, -0.500000f}},
    {{0.500000f,  -0.500000f, -0.500000f}},
    {{0.500000f,  0.500000f, -0.500000f}},
    {{0.500000f,  0.500000f, 0.500000f}},
    {{0.500000f,  -0.500000f, -0.500000f}},
    {{0.500000f,  0.500000f, 0.500000f}},
    {{0.500000f,  -0.500000f, 0.500000f}},
    {{0.500000f,  -0.500000f, 0.500000f}},
    {{0.500000f,  0.500000f, 0.500000f}},
    {{-0.500000f, 0.500000f, 0.500000f}},
    {{0.500000f,  -0.500000f, 0.500000f}},
    {{-0.500000f, 0.500000f, 0.500000f}},
    {{-0.500000f, -0.500000f, 0.500000f}},
    {{-0.500000f, -0.500000f, -0.500000f}},
    {{0.500000f,  -0.500000f, -0.500000f}},
    {{0.500000f,  -0.500000f, 0.500000f}},
    {{-0.500000f, -0.500000f, -0.500000f}},
    {{0.500000f,  -0.500000f, 0.500000f}},
    {{-0.500000f, -0.500000f, 0.500000f}},
    {{0.500000f,  0.500000f, -0.500000f}},
    {{-0.500000f, 0.500000f, -0.500000f}},
    {{-0.500000f, 0.500000f, 0.500000f}},
    {{0.500000f,  0.500000f, -0.500000f}},
    {{-0.500000f, 0.500000f, 0.500000f}},
    {{0.500000f,  0.500000f, 0.500000f}},
};
// clang-format on

static ImmediateDrawQueue::Vertex Cylinder_Vertices[] = {
    {{-1.000000f, 0.000000f, -1.000000f}},
    {{1.000000f, 0.000000f, -1.000000f}},
    {{1.000000f, -0.707107f, -0.707107f}},
    {{-1.000000f, 0.000000f, -1.000000f}},
    {{1.000000f, -0.707107f, -0.707107f}},
    {{-1.000000f, -0.707107f, -0.707107f}},
    {{-1.000000f, -0.707107f, -0.707107f}},
    {{1.000000f, -0.707107f, -0.707107f}},
    {{1.000000f, -1.000000f, 0.000000f}},
    {{-1.000000f, -0.707107f, -0.707107f}},
    {{1.000000f, -1.000000f, 0.000000f}},
    {{-1.000000f, -1.000000f, 0.000000f}},
    {{-1.000000f, -1.000000f, 0.000000f}},
    {{1.000000f, -1.000000f, 0.000000f}},
    {{1.000000f, -0.707107f, 0.707107f}},
    {{-1.000000f, -1.000000f, 0.000000f}},
    {{1.000000f, -0.707107f, 0.707107f}},
    {{-1.000000f, -0.707107f, 0.707107f}},
    {{-1.000000f, -0.707107f, 0.707107f}},
    {{1.000000f, -0.707107f, 0.707107f}},
    {{1.000000f, 0.000000f, 1.000000f}},
    {{-1.000000f, -0.707107f, 0.707107f}},
    {{1.000000f, 0.000000f, 1.000000f}},
    {{-1.000000f, 0.000000f, 1.000000f}},
    {{-1.000000f, 0.000000f, 1.000000f}},
    {{1.000000f, 0.000000f, 1.000000f}},
    {{1.000000f, 0.707107f, 0.707107f}},
    {{-1.000000f, 0.000000f, 1.000000f}},
    {{1.000000f, 0.707107f, 0.707107f}},
    {{-1.000000f, 0.707107f, 0.707107f}},
    {{-1.000000f, 0.707107f, 0.707107f}},
    {{1.000000f, 0.707107f, 0.707107f}},
    {{1.000000f, 1.000000f, 0.000000f}},
    {{-1.000000f, 0.707107f, 0.707107f}},
    {{1.000000f, 1.000000f, 0.000000f}},
    {{-1.000000f, 1.000000f, 0.000000f}},
    {{1.000000f, -0.707107f, -0.707107f}},
    {{1.000000f, 0.000000f, -1.000000f}},
    {{1.000000f, 0.707107f, -0.707107f}},
    {{1.000000f, -0.707107f, -0.707107f}},
    {{1.000000f, 0.707107f, -0.707107f}},
    {{1.000000f, 1.000000f, 0.000000f}},
    {{1.000000f, -0.707107f, -0.707107f}},
    {{1.000000f, 1.000000f, 0.000000f}},
    {{1.000000f, 0.707107f, 0.707107f}},
    {{1.000000f, -0.707107f, -0.707107f}},
    {{1.000000f, 0.707107f, 0.707107f}},
    {{1.000000f, 0.000000f, 1.000000f}},
    {{1.000000f, -0.707107f, -0.707107f}},
    {{1.000000f, 0.000000f, 1.000000f}},
    {{1.000000f, -0.707107f, 0.707107f}},
    {{1.000000f, -0.707107f, -0.707107f}},
    {{1.000000f, -0.707107f, 0.707107f}},
    {{1.000000f, -1.000000f, 0.000000f}},
    {{-1.000000f, 1.000000f, 0.000000f}},
    {{1.000000f, 1.000000f, 0.000000f}},
    {{1.000000f, 0.707107f, -0.707107f}},
    {{-1.000000f, 1.000000f, 0.000000f}},
    {{1.000000f, 0.707107f, -0.707107f}},
    {{-1.000000f, 0.707107f, -0.707107f}},
    {{-1.000000f, 0.707107f, -0.707107f}},
    {{1.000000f, 0.707107f, -0.707107f}},
    {{1.000000f, 0.000000f, -1.000000f}},
    {{-1.000000f, 0.707107f, -0.707107f}},
    {{1.000000f, 0.000000f, -1.000000f}},
    {{-1.000000f, 0.000000f, -1.000000f}},
    {{-1.000000f, 0.000000f, -1.000000f}},
    {{-1.000000f, -0.707107f, -0.707107f}},
    {{-1.000000f, -1.000000f, 0.000000f}},
    {{-1.000000f, 0.000000f, -1.000000f}},
    {{-1.000000f, -1.000000f, 0.000000f}},
    {{-1.000000f, -0.707107f, 0.707107f}},
    {{-1.000000f, 0.000000f, -1.000000f}},
    {{-1.000000f, -0.707107f, 0.707107f}},
    {{-1.000000f, 0.000000f, 1.000000f}},
    {{-1.000000f, 0.000000f, -1.000000f}},
    {{-1.000000f, 0.000000f, 1.000000f}},
    {{-1.000000f, 0.707107f, 0.707107f}},
    {{-1.000000f, 0.000000f, -1.000000f}},
    {{-1.000000f, 0.707107f, 0.707107f}},
    {{-1.000000f, 1.000000f, 0.000000f}},
    {{-1.000000f, 0.000000f, -1.000000f}},
    {{-1.000000f, 1.000000f, 0.000000f}},
    {{-1.000000f, 0.707107f, -0.707107f}},
};

static ImmediateDrawQueue::Vertex Line_Vertices[] = {
    {{0.0f, 0.0f, 0.0f}},
    {{0.0f, 0.0f, -1.0f}},
};

/*

Separate vertex buffer / object

[]

*/

static VkShaderModule load_shader(
    Allocator& allocator, Str path, VkDevice device)
{
    CREATE_SCOPED_ARENA(allocator, temp_alloc, KILOBYTES(8));
    auto load_result = load_shader_binary(temp_alloc, device, path);

    if (!load_result.ok()) {
        print(LIT("Failed to load & create shader module {}\n"), path);
        return VK_NULL_HANDLE;
    }

    print(LIT("Loaded shader module binary {}\n"), path);
    return load_result.value();
}

void ImmediateDrawQueue::init(
    VkDevice               device,
    VkRenderPass           render_pass,
    VMA&                   vma,
    DescriptorLayoutCache* cache,
    DescriptorAllocator*   allocator)
{
    CREATE_SCOPED_ARENA(System_Allocator, temp, KILOBYTES(1));

    pvma         = &vma;
    this->device = device;

    // Global data buffer
    global_buffer =
        VMA_CREATE_BUFFER(
            vma,
            sizeof(GPUCameraData),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU)
            .unwrap();

    // Object data buffer
    object_buffer =
        VMA_CREATE_BUFFER(
            vma,
            sizeof(ImmediateDrawQueue::GPUObjectData) * Max_Objects,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU)
            .unwrap();

    VkDescriptorBufferInfo global_buffer_info = {
        .buffer = global_buffer.buffer,
        .offset = 0,
        .range  = sizeof(ImmediateDrawQueue::GPUCameraData),
    };

    ASSERT(DescriptorBuilder::create(System_Allocator, cache, allocator)
               .bind_buffer(
                   0,
                   &global_buffer_info,
                   VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
               .build(global_set, global_set_layout));

    // Descriptor sets
    VkDescriptorBufferInfo object_buffer_info = {
        .buffer = object_buffer.buffer,
        .offset = 0,
        .range  = sizeof(ImmediateDrawQueue::GPUObjectData) * Max_Objects,
    };

    ASSERT(DescriptorBuilder::create(System_Allocator, cache, allocator)
               .bind_buffer(
                   0,
                   &object_buffer_info,
                   VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
               .build(object_set, object_set_layout));

    // Cube vertex buffer
    {
        const size_t buffer_size = sizeof(Vertex) * ARRAY_COUNT(Cube_Vertices);

        cube_buffer =
            VMA_CREATE_BUFFER(
                vma,
                buffer_size,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VMA_MEMORY_USAGE_CPU_TO_GPU)
                .unwrap();

        u8* data = (u8*)VMA_MAP(vma, cube_buffer);
        memcpy(data, Cube_Vertices, buffer_size);
        VMA_UNMAP(vma, cube_buffer);
    }

    // Cylinder vertex buffer
    {
        const size_t buffer_size =
            sizeof(Vertex) * ARRAY_COUNT(Cylinder_Vertices);

        cylinder_buffer =
            VMA_CREATE_BUFFER(
                vma,
                buffer_size,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VMA_MEMORY_USAGE_CPU_TO_GPU)
                .unwrap();

        u8* data = (u8*)VMA_MAP(vma, cylinder_buffer);
        memcpy(data, Cylinder_Vertices, buffer_size);
        VMA_UNMAP(vma, cylinder_buffer);
    }

    // Line vertex buffer
    {
        const size_t buffer_size = sizeof(Vertex) * ARRAY_COUNT(Line_Vertices);

        line_buffer =
            VMA_CREATE_BUFFER(
                vma,
                buffer_size,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VMA_MEMORY_USAGE_CPU_TO_GPU)
                .unwrap();

        u8* data = (u8*)VMA_MAP(vma, line_buffer);
        memcpy(data, Line_Vertices, buffer_size);
        VMA_UNMAP(vma, line_buffer);
    }

    auto layouts =
        arr<VkDescriptorSetLayout>(global_set_layout, object_set_layout);

    VkPipelineLayoutCreateInfo create_info = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount         = layouts.count(),
        .pSetLayouts            = layouts.elements,
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
        .offset   = (u32)OFFSET_OF(ImmediateDrawQueue::Vertex, position),
    };

    VkShaderModule vertex_shader_mod = load_shader(
        System_Allocator,
        LIT("Shaders/Immediate.vert.spv"),
        device);

    VkShaderModule frag_shader_mod = load_shader(
        System_Allocator,
        LIT("Shaders/Immediate.frag.spv"),
        device);

    pipeline =
        PipelineBuilder(temp)
            .add_shader_stage(VK_SHADER_STAGE_VERTEX_BIT, vertex_shader_mod)
            .add_shader_stage(VK_SHADER_STAGE_FRAGMENT_BIT, frag_shader_mod)
            .add_dynamic_state(VK_DYNAMIC_STATE_VIEWPORT)
            .add_dynamic_state(VK_DYNAMIC_STATE_SCISSOR)
            .set_primitive_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .set_polygon_mode(VK_POLYGON_MODE_FILL)
            .set_render_pass(render_pass)
            .set_layout(pipeline_layout)
            .set_vertex_input_info(vertex_input_info)
            .set_depth_test(false, true, VK_COMPARE_OP_LESS)
            .build(device)
            .unwrap();

    line_pipeline =
        PipelineBuilder(temp)
            .add_shader_stage(VK_SHADER_STAGE_VERTEX_BIT, vertex_shader_mod)
            .add_shader_stage(VK_SHADER_STAGE_FRAGMENT_BIT, frag_shader_mod)
            .add_dynamic_state(VK_DYNAMIC_STATE_VIEWPORT)
            .add_dynamic_state(VK_DYNAMIC_STATE_SCISSOR)
            .set_primitive_topology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST)
            .set_polygon_mode(VK_POLYGON_MODE_LINE)
            .set_render_pass(render_pass)
            .set_layout(pipeline_layout)
            .set_vertex_input_info(vertex_input_info)
            .set_depth_test(false, true, VK_COMPARE_OP_LESS)
            .build(device)
            .unwrap();

    vkDestroyShaderModule(device, vertex_shader_mod, 0);
    vkDestroyShaderModule(device, frag_shader_mod, 0);
}

void ImmediateDrawQueue::deinit()
{
    boxes.release();
    cylinders.release();

    VMA_DESTROY_BUFFER(*pvma, global_buffer);
    VMA_DESTROY_BUFFER(*pvma, object_buffer);
    VMA_DESTROY_BUFFER(*pvma, cube_buffer);
    VMA_DESTROY_BUFFER(*pvma, cylinder_buffer);
    vkDestroyPipelineLayout(device, pipeline_layout, 0);
    vkDestroyPipeline(device, pipeline, 0);
    vkDestroyPipeline(device, line_pipeline, 0);
}

void ImmediateDrawQueue::draw(
    VkCommandBuffer cmd, glm::mat4& view, glm::mat4& proj)
{
    GPUCameraData* camera = (GPUCameraData*)VMA_MAP(*pvma, global_buffer);
    *camera               = {
                      .view     = view,
                      .proj     = proj,
                      .viewproj = proj * view,
    };
    VMA_UNMAP(*pvma, global_buffer);

    GPUObjectData* objects = (GPUObjectData*)VMA_MAP(*pvma, object_buffer);
    u32            c       = 0;

    for (const Box& box : boxes) {
        objects[c] = {
            .matrix = glm::translate(glm::mat4(1.0f), box.center) *
                      glm::mat4(box.rotation.matrix()) *
                      glm::scale(glm::mat4(1.0f), box.extents),
            .color = box.color,
        };
        c++;
    }

    for (const Cylinder& cylinder : cylinders) {
        objects[c] = {
            .matrix = glm::translate(glm::mat4(1.0f), cylinder.center) *
                      // glm::rotate(glm::mat4(1.0f), 0.0f, cylinder.forward) *
                      glm::scale(glm::mat4(1.0f), cylinder.extents),
            .color = glm::vec4(0, 1, 0, 1),
        };
        c++;
    }

    for (const Line& line : lines) {
        Vec3  direction   = normalize(line.end - line.begin);
        Vec3  axis        = cross(Vec3(0, 0, 1), direction);
        float angle       = glm::acos(dot(Vec3(0, 0, 1), direction));
        Mat4  rotation    = glm::rotate(glm::mat4(1.0f), angle, axis);
        Mat4  translation = glm::translate(glm::mat4(1.0f), line.begin);
        float distance =
            glm::distance(glm::vec3(line.begin), glm::vec3(line.end));
        Mat4 scale =
            glm::scale(glm::mat4(1.0f), glm::vec3(1.f, 1.f, -distance));

        objects[c] = {
            .matrix = translation * rotation * scale,
            .color  = line.color,
        };
    }
    VMA_UNMAP(*pvma, object_buffer);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    u32 uniform_offsets[] = {0};
    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline_layout,
        0,
        1,
        &global_set,
        ARRAY_COUNT(uniform_offsets),
        uniform_offsets);

    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline_layout,
        1,
        1,
        &object_set,
        0,
        nullptr);

    // Boxes
    u32 running_instance_offset = 0;

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &cube_buffer.buffer, &offset);
    vkCmdDraw(cmd, ARRAY_COUNT(Cube_Vertices), (u32)boxes.size, 0, 0);
    running_instance_offset += (u32)boxes.size;

    // Cylinders
    vkCmdBindVertexBuffers(cmd, 0, 1, &cylinder_buffer.buffer, &offset);
    vkCmdDraw(
        cmd,
        ARRAY_COUNT(Cylinder_Vertices),
        (u32)cylinders.size,
        0,
        running_instance_offset);
    running_instance_offset += cylinders.size;

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, line_pipeline);
    vkCmdBindVertexBuffers(cmd, 0, 1, &line_buffer.buffer, &offset);
    vkCmdDraw(
        cmd,
        ARRAY_COUNT(Line_Vertices),
        (u32)lines.size,
        0,
        running_instance_offset);
    running_instance_offset += cylinders.size;
}

void ImmediateDrawQueue::clear()
{
    if (lines.size > 100) {
        lines.empty();
    }

    boxes.empty();
    cylinders.empty();
}
