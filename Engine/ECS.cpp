#include "ECS.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Engine.h"
#include "imgui.h"

ECS_COMPONENT_DECLARE(TransformComponent);
ECS_COMPONENT_DECLARE(EditorSelectableComponent);
ECS_COMPONENT_DECLARE(MeshMaterialComponent);

void ECS::init()
{
    // Default components
    {
        ECS_COMPONENT_DEFINE(world, TransformComponent);
        ECS_COMPONENT_DEFINE(world, EditorSelectableComponent);
        ECS_COMPONENT_DEFINE(world, MeshMaterialComponent);
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

    // Rendering queries
    {
        rendering.objects.alloc = &System_Allocator;
        rendering.transform_view_query =
            world.query_builder<TransformComponent, MeshMaterialComponent>()
                .build();
    }

    // REST
    world.set<flecs::Rest>({});
}

flecs::entity ECS::create_entity(Str name)
{
    auto entity = world.entity(name.data);
    entity.set<EditorSelectableComponent>({false});
    entity.set<MeshMaterialComponent>({
        .mesh     = engine->get_mesh(LIT("monke")),
        .material = engine->get_material(LIT("default.mesh")),
    });
    return entity;
}

void ECS::deinit() { ecs_fini(world); }

void ECS::run()
{
    world.progress();

    // Update render objects
    {
        rendering.objects.empty();

        rendering.transform_view_query.each(
            [this](
                flecs::entity          e,
                TransformComponent&    transform,
                MeshMaterialComponent& meshmat) {
                // Compute transform
                glm::mat4 object_transform =
                    glm::translate(glm::mat4(1.0f), transform.position) *
                    glm::toMat4(transform.rotation) *
                    glm::scale(glm::mat4(1.0f), transform.scale);

                rendering.objects.add(RenderObject{
                    .mesh      = meshmat.mesh,
                    .material  = meshmat.material,
                    .transform = object_transform,
                });
            });

        engine->render_objects = slice(rendering.objects);
    }
}

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
                if (comp.raw_id() == ecs_id(TransformComponent)) {
                    auto* t = entity.get_mut<TransformComponent>();

                    glm::vec3 euler =
                        glm::degrees(glm::eulerAngles(t->rotation));

                    ImGui::DragFloat3(
                        "Position",
                        glm::value_ptr(t->position),
                        0.01f);
                    ImGui::DragFloat3("Rotation", glm::value_ptr(euler), 0.01f);
                    ImGui::DragFloat3("Scale", glm::value_ptr(t->scale), 0.01f);

                    t->rotation =
                        glm::angleAxis(
                            glm::radians(euler.z),
                            glm::vec3(0, 0, 1)) *
                        glm::angleAxis(
                            glm::radians(euler.y),
                            glm::vec3(0, 1, 0)) *
                        glm::angleAxis(
                            glm::radians(euler.x),
                            glm::vec3(1, 0, 0));
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
