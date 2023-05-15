#include "WorldRenderSystem.h"

#include "ECS/ECS.h"
#include "Engine.h"
#include "Renderer/Renderer.h"

void WorldRenderSystem::init(Allocator& allocator)
{
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
}

void WorldRenderSystem::deinit() {}

THandle<Mesh> WorldRenderSystem::resolve(const AssetID& id)
{
    THandle<Mesh> result = meshes.get_handle(id);

    if (!result.is_valid()) {
        result = submit_mesh(id);
    }

    return result;
}

THandle<Mesh> WorldRenderSystem::submit_mesh(const AssetID& id)
{
    Engine* eng   = Engine::instance();
    Asset*  asset = eng->asset_system->load_asset_now(id);

    ASSERT(asset != nullptr);

    Mesh new_mesh = Mesh::from_asset(*asset);
    eng->renderer->upload_mesh(new_mesh);

    u32 hid = meshes.create_resource(id, new_mesh);
    return THandle<Mesh>{hid};
}

void WorldRenderSystem::update()
{
    render_objects.empty();

    collection_query.each([this](
                              flecs::entity        e,
                              TransformComponent&  transform,
                              StaticMeshComponent& mesh,
                              MaterialComponent&   material) {
        update_render_object(transform, mesh, material);
    });
}

void WorldRenderSystem::update_render_object(
    TransformComponent&  transform,
    StaticMeshComponent& mesh,
    MaterialComponent&   material)
{
    // glm::mat4 object_transform =
    //     glm::translate(glm::mat4(1.0f), transform.world_position) *
    //     glm::toMat4(transform.world_rotation) *
    //     glm::scale(glm::mat4(1.0f), transform.world_scale);

    // if (mesh.mesh == 0) {
    //     mesh.mesh = renderer->get_mesh(mesh.name);
    // }

    // ASSERT(mesh.mesh);

    // if (material.material == 0) {
    //     material.material =
    //         renderer->material_system.find_material(material.name);
    // }
    // ASSERT(material.material);

    // render_objects.add(RenderObject{
    //     .mesh      = mesh.mesh,
    //     .material  = material.material,
    //     .transform = object_transform,
    // });
}