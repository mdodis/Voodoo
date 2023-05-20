#include "SubsystemManager.h"

void SubsystemManager::init() { subsystems.init(System_Allocator); }

void SubsystemManager::deinit() { subsystems.deinit(); }

void SubsystemManager::_register_subsystem(ISubsystem* subsystem, Str name)
{
    subsystems.create_resource(name, subsystem);
}

ISubsystem* SubsystemManager::_get_subsystem(Str name)
{
    THandle<ISubsystem*> handle = subsystems.get_handle(name);
    return subsystems.resolve(handle);
}
