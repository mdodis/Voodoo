#pragma once
#include "Containers/Array.h"
#include "ECSTypes.h"
#include "WorldSerializer.h"
#include "flecs.h"

struct EntityReference {
    ecs_entity_t id;
};

struct ECS {
    void          init();
    void          deinit();
    void          run();
    flecs::entity create_entity(Str name);

    void open_world(Str path);
    void save_world(Str path);
    void draw_editor();

    WorldSerializer world_serializer;
    flecs::world    world;
    struct Engine*  engine;

    struct {
        flecs::query<EditorSelectableComponent> entity_view_query;
        flecs::query<>                          component_view_query;
        TArray<flecs::entity_t>                 selection = {&System_Allocator};
    } editor;

    struct {
        flecs::query<TransformComponent, MeshMaterialComponent>
                             transform_view_query;
        TArray<RenderObject> objects;
    } rendering;
};
