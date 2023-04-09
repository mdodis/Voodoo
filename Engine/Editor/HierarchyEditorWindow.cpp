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
        [this](flecs::entity e, const EditorSelectableComponent& selectable) {
            auto svname      = e.name();
            bool is_selected = selectable.selected;

            if (ImGui::Selectable(svname.c_str(), is_selected)) {
                flecs::entity_t id = e.id();

                on_entity_clicked(id);

                editor_world().event<events::OnEntitySelectionChanged>()
                    .id<EditorSelectableComponent>()
                    .entity(id)
                    .emit();
            }
        });
}

void HierarchyEditorWindow::deinit() {}

void HierarchyEditorWindow::on_entity_clicked(flecs::entity_t id)
{
    editor_world()
        .filter_builder<EditorSelectableComponent>()
        .build()
        .each(
            [this,
             id](flecs::entity entity, EditorSelectableComponent& selectable) {
                print(LIT("Select {} {}\n"), entity.id(), id);
                auto *m = entity.get_mut<EditorSelectableComponent>();
                m->selected = entity.id() == id;
                
            });
}
