#include "ModuleSystem.h"

#include "ECS/ECS.h"
#include "Engine.h"

void ModuleSystem::init() { Engine* engine = Engine::instance(); }

void ModuleSystem::deinit() {}

void ModuleSystem::load_module(Str name, ProcModuleImport* import_proc)
{
    Engine* engine = Engine::instance();
    print(LIT("[Module System] Loading module {}\n"), name);
    ModuleInitParams params = {
        .systems         = &engine->ecs->system_registrar,
        .components      = &engine->ecs->component_registrar,
        .engine_instance = Engine::instance(),
    };
    import_proc(&params);
}
