#pragma once

#include "ECS.h"

struct Editor {
    struct {
        flecs::query<EditorSelectableComponent> entity_view_query;
        flecs::query<>                          component_view_query;
    } queries;
};

extern Editor The_Editor;