#pragma once
#include <flecs.h>

#include "Containers/Map.h"
#include "Reflection.h"

struct ComponentDescriptor {
    Str          name;
    u32          size;
    i64          alignment;
    IDescriptor* descriptor;
};

struct ComponentDescriptorRegistrar {
    ecs_world_t* world;
    void         init(Allocator& allocator);
    void         deinit();
    void         add(ComponentDescriptor& component);

    TMap<u64, ComponentDescriptor> id_to_desc;
    TMap<Str, u64>                 type_name_to_id;
};