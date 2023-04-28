#include "TestSystem.h"

void move_1_unit_x(flecs::entity entity, TransformComponent& transform)
{
    if (strcmp(entity.name().c_str(), "Entity #1") == 0) {
        transform.position.x += 0.1f;
    }
}