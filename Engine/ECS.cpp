#include "ECS.h"

#include "imgui.h"
#include <glm/gtc/type_ptr.hpp>

ECS_COMPONENT_DECLARE(Transform);
ECS_COMPONENT_DECLARE(EditorSelectableComponent);

void ECS::init()
{
    // Default components
    {
        ECS_COMPONENT_DEFINE(world, Transform);
        ECS_COMPONENT_DEFINE(world, EditorSelectableComponent);
    }

    // Editor Query
    {
        editor.entity_view_query = world.query_builder<EditorSelectableComponent>().build();
    }

    // REST
    world.set<flecs::Rest>({});
}

flecs::entity ECS::create_entity(Str name)
{
    auto entity = world.entity(name.data);
    entity.set<EditorSelectableComponent>({false});
    return entity;
}

void ECS::deinit() { ecs_fini(world); }

void ECS::run() { world.progress(); }

void ECS::draw_editor()
{
    ImGui::Begin("Hierarchy");

    {
        editor.entity_view_query.each(
            [this](flecs::entity e, EditorSelectableComponent& selectable) {
                auto svname = e.name();
                bool is_selected = editor.selection.index_of(e.id()) != NumProps<u64>::max;
                if (ImGui::Selectable(svname.c_str(), is_selected)) {
                    editor.selection.empty();
                    editor.selection.add(e.id());
                }
            });

    }

    ImGui::End();

    ImGui::Begin("Inspector");
    {

        if (editor.selection.size > 0) {

            auto entity = world.get_alive(editor.selection[0]);
            ImGui::Text("%s", entity.name().c_str());
            auto types = entity.type();
            ImGui::PushID(entity.id());
            for (int i = 0; i < types.count(); ++i)
            {
                auto type = types.get(i);
                if (!type.is_entity())
                {
                    continue;
                }

                ImGui::Text("%s", type.str().c_str());

                if (type.raw_id() == ecs_id(Transform))
                {
                    auto *t = entity.get_mut<Transform>();
                    float *v = glm::value_ptr(t->position);
                    ImGui::DragFloat3("Position", v);

                }
                
            }
            ImGui::PopID();
        }
    }
    ImGui::End();
}
