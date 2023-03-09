#include "ECS/World.h"
#include "Test/Test.h"

struct ComponentA {
    int property;
};

struct ComponentB {
    int property;
};

TEST_CASE("Engine/ECS", "Basic world init test")
{
    World world(System_Allocator);
    world.init();

    world.register_native_component_type<ComponentA>();
    world.register_native_component_type<ComponentB>();

    LogicalEntity* en         = world.create_logical_entity();
    auto           a_instance = world.create_component<ComponentA>();
    a_instance->property      = 100;
    world.attach_component_to_entity(a_instance, en);

    return MPASSED();
}
