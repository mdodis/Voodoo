#include "HierarchyEditorWindow.h"

#include "MenuRegistrar.h"
#include "imgui.h"

static StaticMenuItem Hierarchy_Editor_Window_Item(
    LIT("Window/Hierarchy"), Delegate<void>::create_lambda([]() {
        The_Editor.create_window<HierarchyEditorWindow>();
    }));

void HierarchyEditorWindow::init() {}

void HierarchyEditorWindow::draw()
{
    The_Editor.queries.entity_view_query.each(
        [this](flecs::entity e, EditorSelectableComponent& selectable) {
            auto svname      = e.name();
            bool is_selected = false;
            if (ImGui::Selectable(svname.c_str(), is_selected)) {
            }
        });
}

void HierarchyEditorWindow::deinit() {}
