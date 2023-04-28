#pragma once
#include "Editor.h"
#include "ECS.h"

struct InspectorEditorWindow : public EditorWindow {
    virtual Str  name() override { return LIT("Inspector"); }
    virtual void init() override;
    virtual void draw() override;
    virtual void deinit() override;

    flecs::entity_t selected_entity = 0;
};