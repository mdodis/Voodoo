#pragma once
#include "Core/MathTypes.h"
#include "ECS.h"
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
    virtual void begin_style() override;
    virtual void end_style() override;

    u32  viewport_width  = NumProps<u32>::max;
    u32  viewport_height = NumProps<u32>::max;
    Vec2 window_pos;

    Ray  ray_from_mouse_pos();
    Vec3 mouse_pos();

    Vec2 image_screen_pos;

    void* viewport_texture_ref = nullptr;

    flecs::entity_t selected_entity = 0;
    flecs::entity   gizmo_entity;
    void on_change_selection(flecs::entity new_entity);
};
