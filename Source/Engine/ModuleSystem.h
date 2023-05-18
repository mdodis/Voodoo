#pragma once

#include "Module.h"

struct ModuleSystem {
    void init();
    void deinit();

    void load_module(Str name, ProcModuleImport* import_proc);
};