#pragma once
#include <flecs.h>

#include "Reflection.h"

struct ComponentDescriptor {
    Str          name;
    u32          size;
    IDescriptor* descriptor;
};

struct ComponentDescriptorRegistrar {
    ecs_world_t* world;
    void         add(ComponentDescriptor& component);
};