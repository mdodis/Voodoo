#pragma once
#include "AssetSystem.h"
#include "Core/Handle.h"
#include "Renderer/Mesh.h"

struct WorldRenderObject {
    THandle<Mesh>            mesh_handle;
    struct MaterialInstance* material;
    Mat4                     transform;
};

struct WorldRenderSystem {
    void init(Allocator& allocator);
    void deinit();

    THandle<Mesh> resolve(const AssetID& id);

    Mesh* get(THandle<Mesh> handle);

private:
    /**
     * Loads mesh to GPU
     */
    THandle<Mesh>                submit_mesh(const AssetID& id);
    THandleSystem<AssetID, Mesh> meshes;
};