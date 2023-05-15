#pragma once

#include "Containers/Array.h"
#include "ECS.h"
#include "Editor.h"

struct SearchWorldEditorWindow : public EditorWindow {
    virtual Str  name() override { return LIT("Search World"); }
    virtual void init() override;
    virtual void draw() override;
    virtual void deinit() override;

    TArray<flecs::entity_t> search_results;
};