#pragma once
#include "Containers/Array.h"
#include "Containers/Map.h"
#include "Delegates.h"
#include "Memory/Arena.h"
#include "Str.h"

struct MenuRegistrar {
    struct MenuItem {
        Str            name;
        Delegate<void> on_invoke;
        int priority = 0;
    };

    void             register_item(Str name, Delegate<void> on_invoke, int priority);
    void             draw();
    void             init();
    int              rank(const MenuItem& item);
    TArray<MenuItem> menu_items;
    TMap<Str, int>   precedence;
    Arena            temp_arena;
};

MenuRegistrar* get_menu_registrar();

struct StaticMenuItem {
    StaticMenuItem(Str name, Delegate<void> on_invoke, int priority = 0)
    {
        get_menu_registrar()->register_item(name, on_invoke, priority);
    }
};