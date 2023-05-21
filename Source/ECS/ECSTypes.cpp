#include "ECSTypes.h"

#define ALL_DEFAULT_ECS_TYPES    \
    X(EditorSelectableComponent) \
    X(EditorGizmoShapeComponent)

// Use ECS_COMPONENT_DECLARE to declare the actual ecs_id of components
#define X(name) ECS_COMPONENT_DECLARE(name);
ALL_DEFAULT_ECS_TYPES
#undef X

void register_default_ecs_types(flecs::world& world)
{
    // Use ECS_COMPONENT_DEFINE to register the components
#define X(name) ECS_COMPONENT_DEFINE(world, name);
    ALL_DEFAULT_ECS_TYPES
#undef X
}

void register_default_ecs_descriptors(
    Delegate<void, u64, IDescriptor*> callback)
{
#define X(name) callback.call(ecs_id(name), descriptor_of_or_zero((name*)0));
    ALL_DEFAULT_ECS_TYPES
#undef X
}
