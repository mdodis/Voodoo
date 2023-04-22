#include "ECS.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Engine.h"
#include "StringFormat.h"
#include "imgui.h"

void ECS::init()
{
    register_default_ecs_types(world);
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

    // Serializer
    {
        world_serializer.init(System_Allocator);
        auto register_func = Delegate<void, u64, IDescriptor*>::create_lambda(
            [this](u64 id, IDescriptor* desc) {
                world_serializer.register_descriptor(id, desc);
            });
        register_default_ecs_descriptors(register_func);
    }

    world.system<const TransformComponent, TransformComponent>()
        .term_at(1)
        .parent()
        .cascade()
        .optional()
        .iter([this](
                  flecs::iter&              it,
                  const TransformComponent* parent_transform,
                  TransformComponent*       transform) {
            for (auto i : it) {
                if (parent_transform) {
                    Mat4 parent_matrix = Mat4::make_transform(
                        parent_transform->world_position,
                        parent_transform->world_rotation,
                        parent_transform->world_scale);

                    Mat4 child_matrix = Mat4::make_transform(
                        transform[i].position,
                        transform[i].rotation,
                        transform[i].scale);

                    Mat4      result = parent_matrix * child_matrix;
                    glm::vec3 world_position;
                    glm::quat world_rotation;
                    glm::vec3 world_scale;
                    glm::vec3 world_skew;
                    glm::vec4 world_perspective;
                    glm::decompose(
                        glm::mat4(result),
                        world_scale,
                        world_rotation,
                        world_position,
                        world_skew,
                        world_perspective);

                    transform[i].world_position = Vec3(world_position);
                    transform[i].world_rotation = Quat(world_rotation);
                    transform[i].world_scale    = Vec3(world_scale);

                } else {
                    transform[i].world_position = Vec3(transform->position);
                    transform[i].world_rotation = Quat(transform->rotation);
                    transform[i].world_scale    = Vec3(transform->scale);
                }
            }
        });
}

flecs::entity ECS::create_entity(Str name)
{
    flecs::entity entity;
    if (name != Str::NullStr) {
        entity = world.entity(name.data);
    } else {
        CREATE_SCOPED_ARENA(&System_Allocator, temp, 1024);

        bool created = false;
        u32  i       = 0;
        while (!created) {
            SAVE_ARENA(temp);

            if (i == 0) {
                name = format(temp, LIT("New Entity\0"));
            } else {
                name = format(temp, LIT("New Entity #{}\0"), i);
            }

            if (!world.lookup(name.data)) {
                entity  = world.entity(name.data);
                created = true;
            } else {
                i++;
            }
        }
    }

    entity.set<EditorSelectableComponent>({false, false});
    return entity;
}

void ECS::deinit()
{
    world_serializer.deinit();
    ecs_fini(world);
}

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
                    glm::translate(glm::mat4(1.0f), transform.world_position) *
                    glm::toMat4(transform.world_rotation) *
                    glm::scale(glm::mat4(1.0f), transform.world_scale);

                if (meshmat.mesh == 0) {
                    meshmat.mesh = engine->get_mesh(meshmat.mesh_name);
                }

                ASSERT(meshmat.mesh);

                if (meshmat.material == 0) {
                    meshmat.material =
                        engine->get_material(meshmat.material_name);
                }
                ASSERT(meshmat.material);

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

void ECS::defer(Delegate<void>&& delegate)
{
    world.defer([delegate]() { delegate.call(); });
}

void ECS::open_world(Str path)
{
    world.delete_with<EditorSelectableComponent>();
    world_serializer.import(world, path);
}

void ECS::save_world(Str path) { world_serializer.save(world, path); }
