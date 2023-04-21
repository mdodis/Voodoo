#include "HierarchyEditorWindow.h"

#include "MenuRegistrar.h"
#include "imgui.h"

STATIC_MENU_ITEM(
    "Window/Hierarchy",
    { The_Editor.create_window<HierarchyEditorWindow>(); },
    0);

void HierarchyEditorWindow::begin_style()
{
    ImVec2 padding = ImGui::GetStyle().WindowPadding;
    padding.x      = 0.0f;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, padding);
}
void HierarchyEditorWindow::end_style() { ImGui::PopStyleVar(); }

static int flecs_ordering(
    flecs::entity_t                  a,
    const EditorSelectableComponent* ac,
    flecs::entity_t                  b,
    const EditorSelectableComponent* bc)
{
    return 0;
}

void HierarchyEditorWindow::init()
{
    query = ecs()
                .world.query_builder<EditorSelectableComponent>()
                .expr("(ChildOf, 0), ?Disabled")
                .build();
}

void HierarchyEditorWindow::draw_entity(
    flecs::entity entity, EditorSelectableComponent& selectable)
{
    if (!entity.is_entity()) return;

    bool            is_selected = selectable.selected;
    auto            svname      = entity.name();
    flecs::entity_t eid         = entity.id();
    bool            is_leaf     = ecs().world.count(flecs::ChildOf, eid) == 0;

    const ImGuiTreeNodeFlags base_flags =
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick |
        ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_NoTreePushOnOpen |
        ImGuiTreeNodeFlags_FramePadding;
    ImGuiTreeNodeFlags tree_flags = base_flags;

    if (is_leaf) tree_flags |= ImGuiTreeNodeFlags_Leaf;

    if (is_selected) tree_flags |= ImGuiTreeNodeFlags_Selected;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 2));

    selectable.show_children = ImGui::TreeNodeEx(svname.c_str(), tree_flags);

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        on_entity_clicked(eid);
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleVar();

    if (selectable.show_children) {
        ImGui::Indent();

        auto filter =
            ecs()
                .world.filter_builder<EditorSelectableComponent>()
                .term(flecs::ChildOf, eid)
                .term_at(1)
                .with(flecs::Disabled)
                .optional()
                .build();

        filter.each([this](
                        flecs::entity              child,
                        EditorSelectableComponent& child_selectable) {
            draw_entity(child, child_selectable);
        });
        ImGui::Unindent();
    }
}

void HierarchyEditorWindow::draw()
{
    if (ImGui::BeginPopupContextWindow("##HierarchyEditorWindowContextMenu")) {
        if (ImGui::MenuItem("Create Entity")) {
            ecs().create_entity(Str::NullStr);
        }

        ImGui::EndPopup();
    }

    query.each(
        [this](flecs::entity entity, EditorSelectableComponent& selectable) {
            draw_entity(entity, selectable);
        });

#if 0
    query.iter([this](
                   flecs::iter&                     it,
                   EditorSelectableComponent*       selectable,
                   const EditorSelectableComponent* parent_selectable) {
        for (auto i : it) {
            flecs::entity e = it.entity(i);
            if (parent_selectable) {
                if (!parent_selectable->show_children) return;
            }
            bool is_selected = selectable[i].selected;
            auto svname      = e.name();

            bool is_leaf = false;
            int  depth   = e.depth(flecs::ChildOf);

            const ImGuiTreeNodeFlags base_flags =
                ImGuiTreeNodeFlags_OpenOnArrow |
                ImGuiTreeNodeFlags_OpenOnDoubleClick |
                ImGuiTreeNodeFlags_SpanFullWidth |
                ImGuiTreeNodeFlags_NoTreePushOnOpen |
                ImGuiTreeNodeFlags_FramePadding;
            ImGuiTreeNodeFlags tree_flags = base_flags;

            flecs::entity_t eid = e.id();
            if (is_leaf) tree_flags |= ImGuiTreeNodeFlags_Leaf;

            if (is_selected) tree_flags |= ImGuiTreeNodeFlags_Selected;

            for (int i = 0; i < depth; ++i) ImGui::Indent();

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 2));

            selectable[i].show_children =
                ImGui::TreeNodeEx(e.name().c_str(), tree_flags);

            ImGui::PopStyleVar();
            ImGui::PopStyleVar();

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Depth %d", depth);
            }

            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                on_entity_clicked(eid);
            }

            // if (selectable[i].show_children) {
            //     ImGui::TreePop();
            // }

            for (int i = 0; i < depth; ++i) ImGui::Unindent();
        }
    });
    query.each([this](
                   flecs::entity              e,
                   EditorSelectableComponent& selectable,
                   EditorSelectableComponent* parent_selectable) {
        if (parent_selectable) {
            if (!parent_selectable->show_children) return;
        }
        bool is_selected = selectable.selected;
        auto svname      = e.name();

        bool                     is_leaf = e.has(flecs::ChildOf);
        const ImGuiTreeNodeFlags base_flags =
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_OpenOnDoubleClick |
            ImGuiTreeNodeFlags_SpanAvailWidth;
        ImGuiTreeNodeFlags tree_flags = base_flags;

        flecs::entity_t eid = e.id();

        if (is_selected) tree_flags |= ImGuiTreeNodeFlags_Selected;

        selectable.show_children =
            ImGui::TreeNodeEx(e.name().c_str(), tree_flags);

        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            on_entity_clicked(eid);
        }
    });
#endif
}

void HierarchyEditorWindow::deinit() {}

void HierarchyEditorWindow::on_entity_clicked(flecs::entity_t id)
{
    editor_world()
        .filter_builder<EditorSelectableComponent>()
        .oper(flecs::Or)
        .with(flecs::Disabled)
        .build()
        .each(
            [this,
             id](flecs::entity entity, EditorSelectableComponent& selectable) {
                print(LIT("Select {} {}\n"), entity.id(), id);
                auto* m     = entity.get_mut<EditorSelectableComponent>();
                m->selected = entity.id() == id;
            });

    editor_world()
        .event<events::OnEntitySelectionChanged>()
        .id<EditorSelectableComponent>()
        .entity(id)
        .emit();
}
