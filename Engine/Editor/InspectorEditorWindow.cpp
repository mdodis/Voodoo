#include "InspectorEditorWindow.h"

#include <glm/gtc/type_ptr.hpp>

#include "MenuRegistrar.h"
#include "imgui.h"

STATIC_MENU_ITEM(
    "Window/Inspector",
    { The_Editor.create_window<InspectorEditorWindow>(); },
    0);

void InspectorEditorWindow::init()
{
    editor_world()
        .observer<const EditorSelectableComponent>()
        .event<events::OnEntitySelectionChanged>()
        .oper(flecs::Or)
        .with(flecs::Disabled)
        .each([this](
                  flecs::iter&                     it,
                  size_t                           i,
                  const EditorSelectableComponent& sel) {
            selected_entity = it.entity(i);
        });
}

void InspectorEditorWindow::draw()
{
    flecs::entity entity = editor_world().get_alive(selected_entity);
    if (!entity.is_valid()) return;

    ImGui::PushID(entity.id());

    ImGui::BeginGroup();
    bool is_entity_enabled = entity.enabled();

    if (ImGui::Checkbox("##enable", &is_entity_enabled)) {
        if (is_entity_enabled) {
            entity.enable();
        } else {
            auto* comp     = entity.get_mut<EditorSelectableComponent>();
            comp->selected = false;
            entity.disable();
        }
    }

    ImGui::SameLine();
    static char name_buf[1024];
    strcpy(name_buf, entity.name().c_str());
    if (ImGui::InputText("##name", name_buf, sizeof(name_buf))) {
        auto existing = ecs().world.lookup(name_buf);

        if (existing && (existing.id() != entity.id())) {
            ImGui::TextColored(
                ImVec4(1.0f, 0, 0, 0.7f),
                "%s already exists",
                name_buf);
        } else {
            entity.set_name(name_buf);
        }
    }

    ImGui::SameLine();
    ImGui::Text("%u", entity.id());
    ImGui::EndGroup();

    ImGui::SeparatorText("Components");

    entity.each([&](flecs::id id) {
        if (!id.is_entity()) return;
        flecs::entity comp = id.entity();
        ImGui::Text("%s", comp.name().c_str());

        if (comp.raw_id() == ecs_id(TransformComponent)) {
            auto* t = entity.get_mut<TransformComponent>();

            glm::vec3 euler = glm::degrees(glm::eulerAngles(t->rotation));

            ImGui::DragFloat3("Position", glm::value_ptr(t->position), 0.01f);
            ImGui::DragFloat3("Rotation", glm::value_ptr(euler), 0.01f);
            ImGui::DragFloat3("Scale", glm::value_ptr(t->scale), 0.01f);

            t->rotation =
                glm::angleAxis(glm::radians(euler.z), glm::vec3(0, 0, 1)) *
                glm::angleAxis(glm::radians(euler.y), glm::vec3(0, 1, 0)) *
                glm::angleAxis(glm::radians(euler.x), glm::vec3(1, 0, 0));

            ImGui::Text(
                "World %f %f %f",
                t->world_position.x,
                t->world_position.y,
                t->world_position.z);
        }
    });
    ImGui::PopID();
}

void InspectorEditorWindow::deinit() {}
