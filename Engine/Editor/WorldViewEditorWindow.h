#pragma once
#include "Core/MathTypes.h"
#include "Editor.h"
#include "ECS.h"

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

    u32  viewport_width  = NumProps<u32>::max;
    u32  viewport_height = NumProps<u32>::max;
    Vec2 window_pos;

    Ray  ray_from_mouse_pos();
    Vec3 mouse_pos();

    void* viewport_texture_ref = nullptr;

    flecs::entity_t selected_entity = 0;
};
