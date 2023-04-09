#pragma once
#include "Containers/Array.h"
#include "Delegates.h"
#include "Memory/Arena.h"
#include "Str.h"

struct MenuRegistrar {
    struct MenuItem {
        Str            name;
        Delegate<void> on_invoke;
    };

    void             register_item(Str name, Delegate<void> on_invoke);
    void             draw();
    void             init();
    TArray<MenuItem> menu_items;
    Arena            temp_arena;
};

MenuRegistrar* get_menu_registrar();

struct StaticMenuItem {
    StaticMenuItem(Str name, Delegate<void> on_invoke)
    {
        get_menu_registrar()->register_item(name, on_invoke);
    }
};