#pragma once
#include "ECS/SystemDescriptor.h"

#define PROC_MODULE_IMPORT(name) void name(ModuleInitParams* params)
PROC_MODULE_IMPORT(ProcModuleImport);

struct Module {
    Str name;
};

struct ModuleInitParams {
    SystemDescriptorRegistrar* systems;
};