#include "SearchWorldEditorWindow.h"

#include <imgui.h>

#include "MenuRegistrar.h"

void SearchWorldEditorWindow::init()
{
    search_results.alloc = &System_Allocator;
}

void SearchWorldEditorWindow::draw()
{
    static thread_local char buffer[1024];

    ImGui::InputText("##search", buffer, sizeof(buffer));

    ImGui::SameLine();
    if (ImGui::Button("Run")) {
        ecs_filter_desc_t filter_desc = {
            .expr = buffer,
        };

        ecs_rule_t* rule = ecs_rule_init(ecs().world.m_world, &filter_desc);

        if (rule) {
            search_results.empty();
            ecs_iter_t it = ecs_rule_iter(ecs().world.m_world, rule);

            while (ecs_rule_next(&it)) {
                for (int i = 0; i < it.count; ++i) {
                    ecs_entity_t entity = it.entities[i];
                    search_results.add(entity);
                }
            }

            ecs_rule_fini(rule);
        }
    }

    ImGui::Text("%llu Results", search_results.size);
    static ImGuiTableFlags flags =
        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable |
        ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;

    if (ImGui::BeginTable("##QueryResultsTable", 3, 0)) {
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn(
            "Description",
            ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        for (flecs::entity_t eid : search_results) {
            if (eid == 0) continue;
            flecs::entity entity(ecs().world, eid);
            if (!entity.is_valid()) continue;

            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%llu", eid);

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", entity.name().c_str());

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", entity.type().str().c_str());
        }
        ImGui::EndTable();
    }
}

void SearchWorldEditorWindow::deinit() { search_results.release(); }

STATIC_MENU_ITEM(
    "Tools/Search World",
    { The_Editor.create_window<SearchWorldEditorWindow>(); },
    0);
