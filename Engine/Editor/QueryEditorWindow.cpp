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

    for (flecs::entity_t eid : query_results) {
        if (eid == 0) continue;

        flecs::entity entity(ecs().world, eid);
        ImGui::Text(entity.name().c_str());
    }
}

void QueryEditorWindow::deinit() { query_results.release(); }

STATIC_MENU_ITEM(
    "Help/Query Runner", { The_Editor.create_window<QueryEditorWindow>(); }, 0);
