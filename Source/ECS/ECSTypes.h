#pragma once
#include "Core/Color.h"
#include "Core/MathTypes.h"
#include "Delegates.h"
#include "Reflection.h"
#include "Renderer/RenderObject.h"
#include "Str.h"
#include "flecs.h"

// Editor-only Components (Not Serialized)

struct EditorSelectableComponent {
    bool selected;
    bool show_children;
    int  view_priority;
};
extern ECS_COMPONENT_DECLARE(EditorSelectableComponent);

struct EditorGizmoShapeComponent {
    enum class ShapeKind
    {
        Box,
    };

    ShapeKind kind;
    Color     color;
    bool      hovered;
    union {
        struct {
            Vec3 extents;
        } obb;
    };

    static _inline EditorGizmoShapeComponent make_obb(float x, float y, float z)
    {
        return EditorGizmoShapeComponent{
            .kind    = ShapeKind::Box,
            .color   = Color{x, y, z, 1.0f},
            .hovered = false,
            .obb =
                {
                    .extents = Vec3(x, y, z),
                },
        };
    }
};
extern ECS_COMPONENT_DECLARE(EditorGizmoShapeComponent);

struct EditorGizmoDraggableComponent {
    Vec3 drag_axis;
    Vec2 mouse_start;
    Vec3 click_position;
    Vec3 initial_position;
};
extern ECS_COMPONENT_DECLARE(EditorGizmoDraggableComponent);

void register_default_ecs_types(flecs::world& world);
void register_default_ecs_descriptors(
    Delegate<void, u64, IDescriptor*> callback);
