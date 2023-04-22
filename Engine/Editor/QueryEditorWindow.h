#pragma once
#include "Containers/Array.h"
#include "ECS.h"
#include "Editor.h"

struct QueryEditorWindow : public EditorWindow {
    virtual Str  name() override { return LIT("Query (Filter)"); }
    virtual void init() override;
    virtual void draw() override;
    virtual void deinit() override;

    TArray<flecs::entity_t> query_results;
};