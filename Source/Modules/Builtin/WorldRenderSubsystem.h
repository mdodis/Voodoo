#pragma once
#include "Builtin.h"
#include "Containers/Array.h"
#include "Core/Handle.h"
#include "ECS.h"
#include "Engine/AssetSystem.h"
#include "Engine/SubsystemManager.h"
#include "Renderer/Mesh.h"
#include "Renderer/RenderObject.h"

struct WorldRenderSubsystem : ISubsystem {
    void init() override;
    void deinit() override;

    THandle<Mesh> resolve(const AssetID& id);
    Mesh*         get(THandle<Mesh> handle);

    void update(struct Engine* engine);

private:
    flecs::query<TransformComponent, StaticMeshComponent2, MaterialComponent2>
        collection_query;

    void update_render_object(
        TransformComponent&   transform,
        StaticMeshComponent2& mesh,
        MaterialComponent2&   material);

    /**
     * Loads mesh to GPU
     */
    THandle<Mesh>                submit_mesh(const AssetID& id);
    THandleSystem<AssetID, Mesh> meshes;

    TArray<RenderObject> render_objects;
};