#pragma once
#include "ECS.h"
#include "Editor.h"

struct ModelToVertexArrayEditorWindow : public EditorWindow {
    virtual Str name() override { return LIT("Convert model to vertex array"); }
    virtual void init() override;
    virtual void draw() override;
    virtual void deinit() override;
};