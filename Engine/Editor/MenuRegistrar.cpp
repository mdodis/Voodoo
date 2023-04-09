#include "MenuRegistrar.h"

#include "Memory/Extras.h"
#include "Sort.h"
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
    precedence.init(System_Allocator);

    precedence.add(LIT("File"), 200);
    precedence.add(LIT("Help"), -200);
}

int MenuRegistrar::rank(const MenuItem& item) {
    Str first_part = item.name.part(0, item.name.first_of('/'));
    int category_precedence = 0;

    if (precedence.contains(first_part)) {
        category_precedence = precedence[first_part];
    }

    return item.priority + category_precedence;
}

void MenuRegistrar::register_item(
    Str name, Delegate<void> on_invoke, int priority)
{
    ASSERT(name.first_of('/') != name.len);

    MenuRegistrar::MenuItem item = {
        .name      = name,
        .on_invoke = on_invoke,
        .priority  = priority,
    };
    menu_items.add(item);

    auto compare_func =
        sort::CompareFunc<MenuRegistrar::MenuItem>::create_lambda(
            [this](
                const MenuRegistrar::MenuItem& left,
                const MenuRegistrar::MenuItem& right) { 
                    return rank(left) > rank(right);
            });

    auto s = slice(menu_items);
    sort::quicksort(s, compare_func);
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
