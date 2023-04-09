#include "MenuRegistrar.h"

#include "Memory/Extras.h"
#include "imgui.h"

static TArray<Str> tokenize_string(IAllocator& allocator, Str s, char separator)
{
    u64 count = s.count_of(separator);

    TArray<Str> result(&allocator);
    result.init(count);

    u64 p = 0;
    u64 i = s.first_of(separator);

    while (i != s.len) {
        if (s[i] == separator) {
            Str part       = s.part(p, i);
            Str clone_part = format(allocator, LIT("{}\0"), part);
            result.add(clone_part);

            p = i + 1;
        }

        i++;
    }

    Str part       = s.part(p, i);
    Str clone_part = format(allocator, LIT("{}\0"), part);
    result.add(clone_part);

    return result;
}

void MenuRegistrar::init()
{
    menu_items.alloc = &System_Allocator;
    temp_arena       = Arena(&System_Allocator);
}

void MenuRegistrar::register_item(Str name, Delegate<void> on_invoke)
{
    ASSERT(name.first_of('/') != name.len);

    MenuRegistrar::MenuItem item = {
        .name      = name,
        .on_invoke = on_invoke,
    };
    menu_items.add(item);
}

void MenuRegistrar::draw()
{
    if (ImGui::BeginMainMenuBar()) {
        for (MenuItem& item : menu_items) {
            SAVE_ARENA(temp_arena);

            TArray<Str> parts = tokenize_string(temp_arena, item.name, '/');
            ASSERT(parts.size > 1);

            u64 c = 0;

            for (u64 i = 0; i < (parts.size - 1); ++i) {
                if (!ImGui::BeginMenu(parts[i].data)) break;
                c++;
            }

            if (c == (parts.size - 1)) {
                Str action = *parts.last();

                if (ImGui::MenuItem(action.data)) {
                    item.on_invoke.call();
                }
            }

            for (u64 i = 0; i != c; ++i) {
                ImGui::EndMenu();
            }
        }
        ImGui::EndMainMenuBar();
    }
}

static MenuRegistrar* Menu_Registrar = 0;

MenuRegistrar* get_menu_registrar()
{
    if (!Menu_Registrar) {
        Menu_Registrar = alloc<MenuRegistrar>(System_Allocator);
        Menu_Registrar->init();
    }

    return Menu_Registrar;
}
