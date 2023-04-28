#pragma once
#include "Editor/Editor.h"

struct HierarchyEditorWindow : public EditorWindow {
    virtual Str  name() override { return LIT("Hierarchy"); }
    virtual void init() override;
    virtual void draw() override;
    virtual void deinit() override;
    virtual void begin_style() override;
    virtual void end_style() override;

    void on_entity_clicked(flecs::entity_t id);
    flecs::query<EditorSelectableComponent> query;

    void draw_entity(
        flecs::entity entity, EditorSelectableComponent& selectable);
};
