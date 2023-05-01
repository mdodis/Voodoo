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

        const int monke_count = 10;

        flecs::entity last_monke;
        CREATE_SCOPED_ARENA(System_Allocator, temp, KILOBYTES(1));
        for (int i = 0; i < monke_count; ++i) {
            SAVE_ARENA(temp);

            Str  entity_name = format(temp, LIT("Entity #{}\0"), i);
            auto ent         = G.engine.ecs->create_entity(entity_name);
            ent.set<TransformComponent>(
                {{i * 5.0f, 0, 0}, {1, 0, 0, 0}, {1, 1, 1}});
            ent.set<MeshMaterialComponent>({
                .mesh_name     = LIT("monke"),
                .material_name = LIT("default.mesh"),
            });
            ent.child_of(empire);

            if (i == 5) last_monke = ent;
        }

        {
            SAVE_ARENA(temp);

            Str  entity_name = format(temp, LIT("Entity #{}\0"), monke_count);
            auto ent         = G.engine.ecs->create_entity(entity_name);
            ent.set<TransformComponent>(
                {{monke_count * 5.0f, 0, 0}, {1, 0, 0, 0}, {1, 1, 1}});
            ent.set<MeshMaterialComponent>({
                .mesh_name     = LIT("monke"),
                .material_name = LIT("default.mesh"),
            });
            ent.child_of(last_monke);
        }
    }

    G.engine.loop();
    G.engine.deinit();
    return 0;
}