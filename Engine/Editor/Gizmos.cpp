#include "Gizmos.h"

#include <imgui.h>

#include "Editor.h"
#include "Editor/WorldViewEditorWindow.h"
#include "Engine.h"

namespace ed {
    namespace gizmos {

        // @todo: push each interactive gizmo into a buffer, and query them all
        // separately at some other point since right now, multiple gizmos can
        // be activated
        enum class Axis
        {
            X,
            Y,
            Z,
        };

        static struct {
            Vec3 mouse_start;
            Vec3 object_start;
            bool dragging  = false;
            Axis drag_axis = Axis::X;
        } Context;

        bool box(
            Vec3  position,
            Quat  rotation,
            Vec3  extents,
            Color default_color,
            Color hovered_color,
            Vec3& intersection)
        {
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

            bool  hovered = false;
            Color color   = default_color;

            if (intersect_ray_obb(r, obb)) {
                color   = hovered_color;
                hovered = true;
            }
            imm.box(position, rotation, extents, color);
            return hovered;
        }

        void translation_axis(Vec3& position, Quat rotation, Axis axis)
        {
            WorldViewEditorWindow* window =
                The_Editor.find_window<WorldViewEditorWindow>();

            Vec3  axis_extents;
            Vec3  axis_direction;
            Vec3  axis_perpendicular;
            Color color;
            switch (axis) {
                case Axis::X: {
                    color              = Color::red();
                    axis_extents       = Vec3(2.0f, 0.1f, 0.1f);
                    axis_direction     = Vec3::right();
                    axis_perpendicular = Vec3::up();
                } break;
                case Axis::Y: {
                    color              = Color::green();
                    axis_extents       = Vec3(0.1f, 2.0f, 0.1f);
                    axis_direction     = Vec3::up();
                    axis_perpendicular = Vec3::right();
                } break;
                case Axis::Z: {
                    color              = Color::blue();
                    axis_extents       = Vec3(0.1f, 0.1f, 2.0f);
                    axis_direction     = Vec3::forward();
                    axis_perpendicular = Vec3::up();
                } break;
            }

            Vec3 box_position = position + (rotation * axis_direction) * 1.0f;
            Vec3 intersection_point;
            if (box(box_position,
                    rotation,
                    axis_extents,
                    color,
                    Color::white(), intersection_point))
            {
                if (ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
                    !Context.dragging)
                {
                    Context.mouse_start  = window->mouse_pos();
                    Context.object_start = position;
                    Context.dragging     = true;
                    Context.drag_axis    = axis;
                }
            }

            if (Context.dragging && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                if (Context.drag_axis != axis) return;

                auto&   imm    = The_Editor.immediate_draw_queue();
                Engine* engine = The_Editor.host.engine;

#if 0
                Vec3 pn = normalize(cross(
                    (rotation * axis_direction),
                    (rotation * axis_perpendicular)));
#else
                Vec3 pn = normalize(cross(
                    axis_direction,
                    position - Vec3(engine->debug_camera.position)));
                pn      = normalize(cross(axis_direction, pn));
#endif

                Plane p(pn, position);
                Ray   r = window->ray_from_mouse_pos();

                Vec3 intersection;
                if (intersect_ray_plane(r, p, intersection)) {
                    Vec3 new_pos = intersection;
                    new_pos      = project(
                                  new_pos - position,
                                  normalize(rotation * axis_direction)) +
                              position;
                    position = new_pos - normalize(rotation * axis_direction);

                    imm.box(
                        intersection,
                        Quat::identity(),
                        Vec3(0.1f, 0.1f, 0.1f),
                        color);
                }
            } else {
                Context.dragging = false;
            }
        }

        void translation_axis_x(Vec3& position, Quat rotation)
        {
            WorldViewEditorWindow* window =
                The_Editor.find_window<WorldViewEditorWindow>();

            Vec3 box_position = position + (rotation * Vec3::right()) * 1.0f;
            if (box(box_position,
                    rotation,
                    Vec3(2.0f, 0.1f, 0.1f),
                    Color(0.66f, 0.00f, 0.13f, 1.00f),
                    Color::white()))
            {
                if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                    Context.mouse_start  = window->mouse_pos();
                    Context.object_start = position;
                    Context.dragging     = true;
                }
            }

            if (Context.dragging && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                Vec3 pn = normalize(
                    cross((rotation * Vec3::right()), (rotation * Vec3::up())));

                Plane p(pn, position);
                Ray   r = window->ray_from_mouse_pos();

                Vec3 intersection;
                if (intersect_ray_plane(r, p, intersection)) {
                    Vec3 new_pos =
                        intersection - (rotation * Vec3::right()) * 1.0f;
                    new_pos =
                        project(new_pos, normalize(rotation * Vec3::right()));
                    position = new_pos;
                }
            } else {
                Context.dragging = false;
            }
        }

        void translation(Vec3& position, Quat rotation)
        {
            translation_axis(position, rotation, Axis::X);
            translation_axis(position, rotation, Axis::Y);
            translation_axis(position, rotation, Axis::Z);
        }

    }  // namespace gizmos
}  // namespace ed
