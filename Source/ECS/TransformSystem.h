#pragma once
#include "ECS.h"
#include "TransformSystem.generated.h"

#if METADESK
// clang-format off

@system(OnUpdate)
apply_transform: {
    @access(in)
    @source(id: this, trav: ChildOf, flags: (up, cascade))
    @op(optional)
    parent: TransformComponent;

    transform: TransformComponent;
}

// clang-format on
#endif