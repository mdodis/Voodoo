#include "ECS.h"
#include "TestSystem.generated.h"

#if METADESK
// clang-format off

@system(OnUpdate) move_1_unit_x:
{
    @access(in) transform : TransformComponent;
    @parent() another : AnotherComponent;
}

@system(OnUpdate) apply_transform:
{
    @access(in)
    @source(id: this, trav: ChildOf, flags: (up, cascade))
    parent : TransformComponent;

    @access(inout)
    transform: TransformComponent;
}

// clang-format on
#endif