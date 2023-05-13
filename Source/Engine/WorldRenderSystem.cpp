#include "WorldRenderSystem.h"

#include "Engine.h"
#include "Renderer/Renderer.h"

void WorldRenderSystem::init(Allocator& allocator) { meshes.init(allocator); }

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
