#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Containers/Array.h"
#include "RenderObject.h"
#include "Str.h"
#include "flecs.h"

struct EntityReference {
    ecs_entity_t id;
};

struct TransformComponent {
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;
};
extern ECS_COMPONENT_DECLARE(TransformComponent);

struct EditorSelectableComponent {
    bool selected;
};
extern ECS_COMPONENT_DECLARE(EditorSelectableComponent);

struct MeshMaterialComponent {
    Mesh*     mesh;
    Material* material;
};
extern ECS_COMPONENT_DECLARE(MeshMaterialComponent);

struct ECS {
    flecs::world   world;
    struct Engine* engine;

    void          init();
    void          deinit();
    void          run();
    flecs::entity create_entity(Str name);

    void save_world(Str path);

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

    void draw_editor();
};