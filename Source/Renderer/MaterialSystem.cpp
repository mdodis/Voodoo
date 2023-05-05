#include "MaterialSystem.h"

#include "Containers/Array.h"
#include "Renderer.h"

void MaterialSystem::init(Renderer* renderer)
{
    owner = renderer;

    CREATE_SCOPED_ARENA(System_Allocator, temp_alloc, KILOBYTES(1));

    VertexInputInfo vertex_input_info = Vertex::get_input_info(temp_alloc);

    // Build default templates
    {
        forward_builder = PipelineBuilder(System_Allocator);
        forward_builder.set_vertex_input_info(vertex_input_info);
        forward_builder.set_primitive_topology(
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        forward_builder.set_polygon_mode(VK_POLYGON_MODE_FILL);
        forward_builder
            .set_depth_test(true, true, VK_COMPARE_OP_GREATER_OR_EQUAL);
        forward_builder.set_cull_mode(VK_CULL_MODE_NONE);
        forward_builder.add_dynamic_state(VK_DYNAMIC_STATE_VIEWPORT);
        forward_builder.add_dynamic_state(VK_DYNAMIC_STATE_SCISSOR);
    }

    // Build effects
    {
        ShaderEffect* textured_lit = build_effect(
            LIT("Shaders/tri_mesh_ssbo_instanced.vert.spv"),
            LIT("Shaders/textured_lit.frag.spv"));

        ShaderPass* textured_lit_pass = build_shader(
            renderer->color_pass.render_pass,
            forward_builder,
            textured_lit);
    }
}

ShaderEffect* MaterialSystem::build_effect(Str vert_path, Str frag_path)
{
    ShaderEffect* effect = alloc<ShaderEffect>(System_Allocator);

    effect->init(System_Allocator);
    effect->add_stage(
        owner->shader_cache.get_shader(vert_path),
        VK_SHADER_STAGE_VERTEX_BIT);
    if (frag_path != Str::NullStr) {
        effect->add_stage(
            owner->shader_cache.get_shader(frag_path),
            VK_SHADER_STAGE_FRAGMENT_BIT);
    }

    auto overrides =
        arr<ShaderEffect::MetadataOverride>(ShaderEffect::MetadataOverride{
            LIT("globalData"),
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC});

    effect->reflect_layout(owner->device, slice(overrides));

    return effect;
}

ShaderPass* MaterialSystem::build_shader(
    VkRenderPass render_pass, PipelineBuilder& builder, ShaderEffect* effect)
{
    ShaderPass* result = alloc<ShaderPass>(System_Allocator);
    result->effect     = effect;
    result->layout     = effect->built_layout;

    builder.set_render_pass(render_pass);
    builder.set_effect(effect);
    result->pipeline = builder.build(owner->device).unwrap();
    return result;
}
