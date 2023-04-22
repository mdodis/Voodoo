#include "QueryEditorWindow.h"

#include <imgui.h>

#include "MenuRegistrar.h"

void QueryEditorWindow::init()
{
    query_results.alloc = &System_Allocator;
    query_results.add((flecs::entity_t)0);
}

void QueryEditorWindow::draw()
{
    static char buffer[1024];

    ImGui::InputText("Query", buffer, sizeof(buffer));

    ImGui::SameLine();
    if (ImGui::Button("Run")) {
        ecs_filter_desc_t filter_desc = {
            .expr = buffer,
        };

        ecs_filter_t* filter =
            ecs_filter_init(ecs().world.m_world, &filter_desc);

        if (filter) {
            query_results.empty();
            ecs_iter_t it = ecs_filter_iter(ecs().world.m_world, filter);

            while (ecs_filter_next(&it)) {
                for (int i = 0; i < it.count; ++i) {
                    ecs_entity_t entity = it.entities[i];
                    query_results.add(entity);
                }
            }
        }
    }

    ImGui::Text("%u Results", query_results.size);
    static ImGuiTableFlags flags =
        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable |
        ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;

    if (ImGui::BeginTable("##QueryResultsTable", 3, 0)) {
        
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        for (flecs::entity_t eid : query_results) {
            if (eid == 0) continue;
            flecs::entity entity(ecs().world, eid);
            if (!entity.is_valid()) continue;

            ImGui::TableNextRow();


            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%u", eid);
            
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", entity.name().c_str());

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", entity.type().str().c_str());

        }
        ImGui::EndTable();
    }

}

void QueryEditorWindow::deinit() { query_results.release(); }

STATIC_MENU_ITEM(
    "Help/Query Runner", { The_Editor.create_window<QueryEditorWindow>(); }, 0);
