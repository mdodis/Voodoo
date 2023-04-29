#pragma once
#include "Editor.h"

struct ProjectEditorWindow : public EditorWindow {
    virtual Str  name() override { return LIT("Project"); }
    virtual void init() override;
    virtual void draw() override;
    virtual void deinit() override;
};