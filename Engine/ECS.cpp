#include "ECS.h"

#include <glm/gtc/type_ptr.hpp>

#include "imgui.h"

ECS_COMPONENT_DECLARE(Transform);
ECS_COMPONENT_DECLARE(EditorSelectableComponent);

void ECS::init()
{
    // Default components
    {
        ECS_COMPONENT_DEFINE(world, Transform);
        ECS_COMPONENT_DEFINE(world, EditorSelectableComponent);
    }

    // Editor queries
    {
        editor.entity_view_query =
            world.query_builder<EditorSelectableComponent>().build();
        editor.component_view_query =
            world.query_builder<flecs::Component>()
                .term(flecs::ChildOf, flecs::Flecs)
                .oper(flecs::Not)
                .self()
                .parent()
                .build();
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
                bool is_selected =
                    editor.selection.index_of(e.id()) != NumProps<u64>::max;
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

            ImGui::PushID(entity.id());
            entity.each([&](flecs::id id) {
                if (!id.is_entity()) return;
                flecs::entity comp = id.entity();
                ImGui::Text("%s", comp.name().c_str());
                if (comp.raw_id() == ecs_id(Transform)) {
                    auto*  t = entity.get_mut<Transform>();
                    float* v = glm::value_ptr(t->position);
                    ImGui::DragFloat3("Position", v);
                }
            });
            ImGui::PopID();
        }
    }
    ImGui::End();

    ImGui::Begin("Types");
    {
        editor.component_view_query.each([](flecs::entity component_type) {
            ImGui::PushID(component_type.id());
            if (ImGui::CollapsingHeader(component_type.name().c_str())) {
            }
            ImGui::PopID();
        });
    }
    ImGui::End();
}
