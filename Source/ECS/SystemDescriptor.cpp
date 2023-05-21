#include "SystemDescriptor.h"

#include "Debugging/Assertions.h"
#include "Defer.h"
#include "StringFormat.h"

void SystemDescriptorRegistrar::add(SystemDescriptor* system)
{
    Str name = format(System_Allocator, LIT("{}\0"), system->name);
    DEFER(System_Allocator.release((umm)name.data));

    ecs_entity_desc_t entity_desc = {
        .name = name.data,
        .add  = {ecs_pair(EcsDependsOn, system->phase), system->phase},
    };

    ecs_entity_t system_entity = ecs_entity_init(world, &entity_desc);

    ecs_system_desc_t desc = {
        .entity = system_entity,
        .query =
            {
                .filter = system->filter_desc,
            },
        .callback       = system->invoke,
        .multi_threaded = system->multi_threaded,
    };

    ASSERT(ecs_system_init(world, &desc) != 0);
}