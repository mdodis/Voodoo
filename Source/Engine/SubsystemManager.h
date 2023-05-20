#pragma once
#include "Containers/Map.h"
#include "Core/Handle.h"

struct ISubsystem {
    virtual void init()   = 0;
    virtual void deinit() = 0;
};

struct SubsystemManager {
    void init();
    void deinit();

    template <typename T>
    void register_subsystem()
    {
        subsystem = alloc<T>(System_Allocator);
        Str name  = Str(typeid(T).name());
        _register_subsystem(subsystem, name);
    }

    template <typename T>
    T* get()
    {
        Str name = Str(typeid(T).name());
        return (T*)_get_subsystem(name);
    }

    void        _register_subsystem(ISubsystem* subsystem, Str name);
    ISubsystem* _get_subsystem(Str name);

    THandleSystem<Str, ISubsystem*> subsystems;
};