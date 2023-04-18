#include "Gizmos.h"

#include <imgui.h>

#include "Editor.h"
#include "Editor/WorldViewEditorWindow.h"
#include "Engine.h"

namespace ed {
    namespace gizmos {
        bool box(Vec3 position, Quat rotation, Vec3 extents)
        {
            // @todo: cache view & proj in scene view first
            auto& imm = The_Editor.immediate_draw_queue();

            WorldViewEditorWindow* window =
                The_Editor.find_window<WorldViewEditorWindow>();

            if (!window) return false;

            Ray r = window->ray_from_mouse_pos();

            OBB obb = {
                .center   = position,
                .rotation = rotation,
                .extents  = extents,
            };

            Color color = Color::red();
            if (intersect_ray_obb(r, obb)) {
                color = Color::white();
            }
            imm.box(position, rotation, extents, color);
            return false;
        }
    }  // namespace gizmos
}  // namespace ed
