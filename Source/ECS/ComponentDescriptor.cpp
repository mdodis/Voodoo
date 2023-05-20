#include "ComponentDescriptor.h"

#include "Debugging/Assertions.h"
#include "FileSystem/Extras.h"

void ComponentDescriptorRegistrar::init(Allocator& allocator)
{
    id_to_desc.init(allocator);
    type_name_to_id.init(allocator);
}

void ComponentDescriptorRegistrar::add(ComponentDescriptor& component)
{
    Str type_name = format(System_Allocator, LIT("{}\0"), component.name);

    ecs_entity_desc_t edesc = {
        .id         = 0,
        .name       = type_name.data,
        .symbol     = type_name.data,
        .use_low_id = true,
    };

    ecs_entity_t eid = ecs_entity_init(world, &edesc);
    ASSERT(eid != 0);

    ecs_component_desc_t cdesc = {
        .entity = eid,
        .type =
            {
                .size      = (int)component.size,
                .alignment = (int)component.alignment,
            },
    };

    ecs_entity_t cid = ecs_component_init(world, &cdesc);
    ASSERT(cid != 0);

    id_to_desc.add(cid, component);
    type_name_to_id.add(type_name, cid);

    print(LIT("[Component Registrar] Registered component {}\n"), type_name);
}