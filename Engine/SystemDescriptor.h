#pragma once
#include "ECS.h"

#define PROC_SYSTEM_DESCRIPTOR_INVOKE(name) void name(ecs_iter_t* it)
typedef PROC_SYSTEM_DESCRIPTOR_INVOKE(ProcSystemDescriptorInvoke);

struct SystemDescriptor {
    Str                         name;
    Str                         phase;
    ecs_filter_desc_t           filter_desc;
    ProcSystemDescriptorInvoke* invoke;
};