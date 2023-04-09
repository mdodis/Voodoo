#include "Editor.h"

#include "Engine.h"
#include "MenuRegistrar.h"
#include "Window/Window.h"
#include "imgui.h"
#include "portable-file-dialogs.h"

void Editor::init(win::Window* host_window, Engine* engine, ECS* ecs)
{
    host.window = host_window;
    host.engine = engine;
    host.ecs    = ecs;

    windows.alloc = &System_Allocator;

    // Editor queries
    {
        queries.entity_view_query =
            ecs->world.query_builder<EditorSelectableComponent>().build();
        queries.component_view_query =
            ecs->world.query_builder<flecs::Component>()
                .term(flecs::ChildOf, flecs::Flecs)
                .oper(flecs::Not)
                .self()
                .parent()
                .build();
    }

    next_window_id = 1;
}

void Editor::draw()
{
    // Begin frame
    host.engine->imgui_new_frame();
    host.window->imgui_new_frame();

    ImGui::NewFrame();

    get_menu_registrar()->draw();

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::DockSpaceOverViewport(
        viewport,
        ImGuiDockNodeFlags_PassthruCentralNode);

    if (imgui_demo) {
        ImGui::ShowDemoWindow(&imgui_demo);
    }

    // Draw windows
    for (EditorWindow* window : windows) {
        if (!window->is_valid()) continue;

        bool is_window_open = true;
        ImGui::Begin(window->name().data, &is_window_open);

        if (is_window_open) {
            window->draw();
        } else {
            window->kill();
        }

        ImGui::End();
    }

    // Free windows with invalid ids
    u64 i = windows.size;
    while (i != 0) {
        i--;
        EditorWindow* window = windows[i];

        if (!window->is_valid()) {
            window->deinit();
            System_Allocator.release((umm)window);
            windows.del(i);
        }
    }
}

void Editor::add_window(EditorWindow* window)
{
    window->id = next_window_id++;
    windows.add(window);
}

void Editor::kill_window(EditorWindow* window) { window->id = 0; }

void Editor::deinit() { windows.release(); }

Editor The_Editor;

/* Default menu items */

static StaticMenuItem File_Quit_Item(
    LIT("File/Quit"), Delegate<void>::create_lambda([]() {
        The_Editor.host.window->is_open = false;
    }));
