#pragma once
#include <flecs.h>

#include "Str.h"

#define PROC_SYSTEM_DESCRIPTOR_INVOKE(name) void name(ecs_iter_t* it)
typedef PROC_SYSTEM_DESCRIPTOR_INVOKE(ProcSystemDescriptorInvoke);

struct SystemDescriptor {
    Str                         name;
    ProcSystemDescriptorInvoke* invoke;
    ecs_filter_desc_t           filter_desc;
    ecs_entity_t                phase;
    bool                        multi_threaded;
};

struct SystemDescriptorRegistrar {
    ecs_world_t* world;
    void         add(SystemDescriptor* system);
};