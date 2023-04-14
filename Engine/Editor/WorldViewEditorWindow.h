#pragma once
#include "Editor.h"

struct WorldViewEditorWindow : public EditorWindow {
    virtual Str  name() override { return LIT("World"); }
    virtual void init() override;
    virtual void draw() override;
    virtual void deinit() override;
};