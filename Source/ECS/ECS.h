#pragma once
#include <flecs.h>

#include "ComponentDescriptor.h"
#include "Containers/Array.h"
#include "Delegates.h"
#include "ECSTypes.h"
#include "SystemDescriptor.h"
#include "WorldSerializer.h"

struct EntityReference {
    ecs_entity_t id;
};

struct ECS {
    void          init(struct Renderer* r);
    void          deinit();
    void          run();
    flecs::entity create_entity(Str name);

    void open_world(Str path);
    void save_world(Str path);

    void defer(Delegate<void>&& delegate);

    WorldSerializer  world_serializer;
    flecs::world     world;
    struct Renderer* renderer;

    SystemDescriptorRegistrar    system_registrar;
    ComponentDescriptorRegistrar component_registrar;

    struct {
        flecs::query<EditorSelectableComponent> entity_view_query;
        flecs::query<>                          component_view_query;
        TArray<flecs::entity_t>                 selection = {&System_Allocator};
    } editor;

    struct {
        TArray<RenderObject> objects;
    } rendering;
};
