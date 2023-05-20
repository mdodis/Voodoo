#pragma once
#include "ECS/ComponentDescriptor.h"
#include "ECS/SystemDescriptor.h"

struct ModuleInitParams {
    SystemDescriptorRegistrar*    systems;
    ComponentDescriptorRegistrar* components;
    struct Engine*                engine_instance;
};

#define PROC_MODULE_IMPORT(name) void name(ModuleInitParams* params)
typedef PROC_MODULE_IMPORT(ProcModuleImport);

struct Module {
    Str name;
};
