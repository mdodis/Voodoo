#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Str.h"
#include "flecs.h"
#include "Containers/Array.h"

struct EntityReference {
    ecs_entity_t id;
};

struct Transform {
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;
};
extern ECS_COMPONENT_DECLARE(Transform);

struct EditorSelectableComponent {
    bool selected;
};
extern ECS_COMPONENT_DECLARE(EditorSelectableComponent);

struct ECS {
    flecs::world  world;
    void          init();
    void          deinit();
    void          run();
    flecs::entity create_entity(Str name);

    struct {
        flecs::query<EditorSelectableComponent> entity_view_query;
        TArray<flecs::entity_t> selection = { &System_Allocator };
    } editor;

    void draw_editor();
};