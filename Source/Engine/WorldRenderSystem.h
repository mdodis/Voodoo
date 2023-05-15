#pragma once
#include "AssetSystem.h"
#include "Containers/Array.h"
#include "Core/Handle.h"
#include "ECS.h"
#include "Renderer/Mesh.h"
#include "Renderer/RenderObject.h"

struct WorldRenderSystem {
    void init(Allocator& allocator);
    void deinit();

    THandle<Mesh> resolve(const AssetID& id);
    Mesh*         get(THandle<Mesh> handle);

    void update();

private:
    flecs::query<TransformComponent, StaticMeshComponent, MaterialComponent>
        collection_query;

    void update_render_object(
        TransformComponent&  transform,
        StaticMeshComponent& mesh,
        MaterialComponent&   material);

    /**
     * Loads mesh to GPU
     */
    THandle<Mesh>                submit_mesh(const AssetID& id);
    THandleSystem<AssetID, Mesh> meshes;

    TArray<RenderObject> render_objects;
};