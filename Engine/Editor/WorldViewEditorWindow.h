#pragma once
#include "Editor.h"

/**
 * @brief The WorldViewEditorWindow class
 *
 * @todo Add support to editor for windows that can only have a single instance,
 * like this one.
 */
struct WorldViewEditorWindow : public EditorWindow {
    virtual Str  name() override { return LIT("World"); }
    virtual void init() override;
    virtual void draw() override;
    virtual void deinit() override;
    virtual bool has_padding() override { return false; }

    u32 viewport_width  = NumProps<u32>::max;
    u32 viewport_height = NumProps<u32>::max;

    void* viewport_texture_ref = nullptr;
};
