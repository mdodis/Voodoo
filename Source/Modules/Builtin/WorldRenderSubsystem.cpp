#include "WorldRenderSubsystem.h"

#include "ECS/ECS.h"
#include "Engine/Engine.h"
#include "Renderer/Renderer.h"

void WorldRenderSubsystem::init()
{
    Allocator& allocator = System_Allocator;
    meshes.init(allocator);
    render_objects.alloc = (&allocator);

    Engine* eng = Engine::instance();

    collection_query =
        eng->ecs->world
            .query_builder<
                TransformComponent,
                StaticMeshComponent,
                MaterialComponent>()
            .build();

    eng->hooks.pre_draw.add_raw(this, &WorldRenderSubsystem::update);
}

void WorldRenderSubsystem::deinit() {}

THandle<Mesh> WorldRenderSubsystem::resolve(const AssetID& id)
{
    THandle<Mesh> result = meshes.get_handle(id);

    if (!result.is_valid()) {
        result = submit_mesh(id);
    }

    return result;
}

Mesh* WorldRenderSubsystem::get(THandle<Mesh> handle)
{
    return &meshes.resolve(handle);
}

THandle<Mesh> WorldRenderSubsystem::submit_mesh(const AssetID& id)
{
    Engine* eng   = Engine::instance();
    Asset*  asset = eng->asset_system->load_asset_now(id);

    ASSERT(asset != nullptr);

    Mesh new_mesh = Mesh::from_asset(*asset);
    eng->renderer->upload_mesh(new_mesh);

    u32 hid = meshes.create_resource(id, new_mesh);
    return THandle<Mesh>{hid};
}

void WorldRenderSubsystem::update(Engine* engine)
{
    render_objects.empty();

    collection_query.each([this](
                              flecs::entity        e,
                              TransformComponent&  transform,
                              StaticMeshComponent& mesh,
                              MaterialComponent&   material) {
        update_render_object(transform, mesh, material);
    });

    engine->renderer->render_objects = slice(render_objects);
}

void WorldRenderSubsystem::update_render_object(
    TransformComponent&  transform,
    StaticMeshComponent& mesh,
    MaterialComponent&   material)
{
    Mat4 object_transform = Mat4::make_translate(transform.world_position) *
                            transform.world_rotation.matrix() *
                            Mat4::make_scale(transform.world_scale);

    Engine* engine = Engine::instance();

    if (!mesh.mesh.is_valid()) {
        mesh.asset.resolve();
        mesh.mesh = resolve(mesh.asset.cached_id);
    }

    Mesh* mesh_ptr = get(mesh.mesh);
    ASSERT(mesh_ptr);

    if (material.instance == nullptr) {
        material.instance =
            engine->renderer->material_system.find_material(material.name);
    }

    ASSERT(material.instance);

    render_objects.add(RenderObject{
        .mesh      = mesh_ptr,
        .material  = material.instance,
        .transform = object_transform,
    });
}