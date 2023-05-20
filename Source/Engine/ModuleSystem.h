#pragma once

#include "Containers/Array.h"
#include "Module.h"

struct ModuleSystem {
    void init();
    void deinit();

    void load_module(Str name, ProcModuleImport* import_proc);
};