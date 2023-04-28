#include "ECS.h"
#include "TestSystem.generated.h"

#if METADESK
// clang-format off

@system(OnUpdate) move_1_unit_x:
{
    transform : TransformComponent;
}

// @system(OnUpdate) apply_transform:
// {
//     @access(in)
//     @source(id: this, trav: ChildOf, flags: (up, cascade))
//     @op(optional)
//     parent : TransformComponent;

//     @access(inout)
//     transform: TransformComponent;
// }

// clang-format on
#endif