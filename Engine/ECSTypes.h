#pragma once
#include "Core/Color.h"
#include "Core/MathTypes.h"
#include "Delegates.h"
#include "Reflection.h"
#include "RenderObject.h"
#include "Str.h"
#include "flecs.h"

using Vec3Descriptor = FixedArrayDescriptor<glm::vec3, float, 3>;
using QuatDescriptor = FixedArrayDescriptor<glm::quat, float, 4>;

/**
 * Transform Component
 *
 * Represents the transform of an entity
 */
struct TransformComponent {
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;

    Vec3 world_position;
    Quat world_rotation;
    Vec3 world_scale;

    static _inline TransformComponent zero()
    {
        return TransformComponent{
            .position = glm::vec3(0, 0, 0),
            .rotation = glm::quat(1, 0, 0, 0),
            .scale    = glm::vec3(1, 1, 1),
        };
    }

    static _inline TransformComponent pos(float x, float y, float z)
    {
        return TransformComponent{
            .position = glm::vec3(x, y, z),
            .rotation = glm::quat(1, 0, 0, 0),
            .scale    = glm::vec3(1, 1, 1),
        };
    }

    _inline Mat4 local_matrix() const
    {
        return Mat4::make_transform(position, rotation, scale);
    }
};
extern ECS_COMPONENT_DECLARE(TransformComponent);

struct TransformComponentDescriptor : public IDescriptor {
    Vec3Descriptor position_desc = {
        OFFSET_OF(TransformComponent, position), LIT("position")};
    QuatDescriptor rotation_desc = {
        OFFSET_OF(TransformComponent, rotation), LIT("rotation")};
    Vec3Descriptor scale_desc = {
        OFFSET_OF(TransformComponent, scale), LIT("scale")};

    IDescriptor* descs[3] = {
        &position_desc,
        &rotation_desc,
        &scale_desc,
    };

    CUSTOM_DESC_OBJECT_DEFAULT(TransformComponent, descs)
};
DEFINE_DESCRIPTOR_OF_INL(TransformComponent);

struct MeshMaterialComponent {
    Str mesh_name;
    Str material_name;

    // Not Serialized
    Mesh*     mesh     = 0;
    Material* material = 0;
};
extern ECS_COMPONENT_DECLARE(MeshMaterialComponent);

struct MeshMaterialComponentDescriptor : public IDescriptor {
    StrDescriptor mesh_name_desc = {
        OFFSET_OF(MeshMaterialComponent, mesh_name), LIT("mesh_name")};
    StrDescriptor material_name_desc = {
        OFFSET_OF(MeshMaterialComponent, material_name), LIT("material_name")};

    IDescriptor* descs[2] = {
        &mesh_name_desc,
        &material_name_desc,
    };

    CUSTOM_DESC_OBJECT_DEFAULT(MeshMaterialComponent, descs)
};
DEFINE_DESCRIPTOR_OF_INL(MeshMaterialComponent);

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
