#include "InspectorEditorWindow.h"

#include "MenuRegistrar.h"
#include "imgui.h"
#include <glm/gtc/type_ptr.hpp>

STATIC_MENU_ITEM(
    "Window/Inspector", { The_Editor.create_window<InspectorEditorWindow>(); },
    0);

void InspectorEditorWindow::init() {
  editor_world()
      .observer<const EditorSelectableComponent>()
      .event<events::OnEntitySelectionChanged>()
      .each([this](flecs::iter &it, size_t i,
                   const EditorSelectableComponent &sel) {
        selected_entity = it.entity(i);
      });
}

void InspectorEditorWindow::draw() {

  flecs::entity entity = editor_world().get_alive(selected_entity);
  if (!entity.is_valid())
    return;

  ImGui::Text("%s", entity.name().c_str());

  ImGui::PushID(entity.id());
  entity.each([&](flecs::id id) {
    if (!id.is_entity())
      return;
    flecs::entity comp = id.entity();
    ImGui::Text("%s", comp.name().c_str());
    if (comp.raw_id() == ecs_id(TransformComponent)) {
      auto *t = entity.get_mut<TransformComponent>();

      glm::vec3 euler = glm::degrees(glm::eulerAngles(t->rotation));

      ImGui::DragFloat3("Position", glm::value_ptr(t->position), 0.01f);
      ImGui::DragFloat3("Rotation", glm::value_ptr(euler), 0.01f);
      ImGui::DragFloat3("Scale", glm::value_ptr(t->scale), 0.01f);

      t->rotation = glm::angleAxis(glm::radians(euler.z), glm::vec3(0, 0, 1)) *
                    glm::angleAxis(glm::radians(euler.y), glm::vec3(0, 1, 0)) *
                    glm::angleAxis(glm::radians(euler.x), glm::vec3(1, 0, 0));
    }
  });
  ImGui::PopID();
}

void InspectorEditorWindow::deinit() {}
