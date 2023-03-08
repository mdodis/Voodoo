#include "ECS/World.h"
#include "Test/Test.h"

struct ComponentA {
    int property;
};

struct ComponentB {
    int property;
};

TEST_CASE("Engine/ECS", "Basic world test") {
    World world(System_Allocator);
    world.init();

    world.register_native_component_type<ComponentA>();
    world.register_native_component_type<ComponentB>();

    LogicalEntity *en = world.create_logical_entity();
    world.create_component();
}
