#pragma once
#include "ECS/ECS.h"
#include "Engine/Engine.h"
#include "Renderer/Renderer.h"

static struct {
    Engine engine;
} G;

int main(int argc, char* argv[])
{
    G.engine.init();

    {
        auto empire = G.engine.ecs->create_entity(LIT("Lost Empire"));
        empire.set<TransformComponent>({{0, 0, 0}, {1, 0, 0, 0}, {1, 1, 1}});
        empire.set<MeshMaterialComponent>({
            .mesh_name     = LIT("lost_empire"),
            .material_name = LIT("default.mesh.textured"),
        });

        empire.set<StaticMeshComponent>({
            .name = LIT("lost_empire"),
        });
        empire.set<MaterialComponent>({
            .name = LIT("lost-empire-albedo"),
        });
    }

    G.engine.loop();
    G.engine.deinit();
    return 0;
}