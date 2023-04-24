// clang-format off
#include "WorldViewEditorWindow.h"
#include "Engine.h"
#include "MenuRegistrar.h"
#include <imgui.h>
#include "Gizmos.h"
#include "backends/imgui_impl_vulkan.h"

// clang-format on

static void update_gizmo_selection(
    flecs::entity                    entity,
    EditorGizmoSelectionData*        data,
    const TransformComponent&        transform,
    const EditorGizmoShapeComponent& shape)
{
    OBB obb = {
        .center   = transform.world_position,
        .rotation = transform.world_rotation,
        .extents  = shape.obb.extents,
    };

    float t1, t2;
    if (intersect_ray_obb(data->ray, obb, t1, t2)) {
        float t = glm::min(t1, t2);
        if (t < data->t) {
            data->t      = t;
            data->hit_id = entity;
        }
    }
}

void WorldViewEditorWindow::init()
{
    Engine& eng      = engine();
    eng.do_blit_pass = false;

    viewport_texture_ref = (void*)ImGui_ImplVulkan_AddTexture(
        eng.present_pass.texture_sampler,
        eng.color_pass.color_image_view,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    editor_world()
        .observer<const EditorSelectableComponent>()
        .event<events::OnEntitySelectionChanged>()
        .each([this](
                  flecs::iter&                     it,
                  size_t                           i,
                  const EditorSelectableComponent& sel) {
            on_change_selection(it.entity(i));
        });

    gizmo_entity = ecs().world.entity("Gizmo");
    gizmo_entity.set(TransformComponent::zero());
    gizmo_entity.set(EditorSyncTransformPositionComponent{});

    {
        auto gizmo_axis = ecs().world.entity("X").child_of(gizmo_entity);
        gizmo_axis.set(TransformComponent::pos(0.5f, 0, 0));
        gizmo_axis.set(EditorGizmoShapeComponent::make_obb(1.0f, 0.1f, 0.1f));
        gizmo_axis.set(
            EditorGizmoDraggableComponent{.drag_axis = Vec3::right()});
    }
    {
        auto gizmo_axis = ecs().world.entity("Y").child_of(gizmo_entity);
        gizmo_axis.set(TransformComponent::pos(0, 0.5f, 0));
        gizmo_axis.set(EditorGizmoShapeComponent::make_obb(0.1f, 1.0f, 0.1f));
        gizmo_axis.set(EditorGizmoDraggableComponent{.drag_axis = Vec3::up()});
    }
    {
        auto gizmo_axis = ecs().world.entity("Z").child_of(gizmo_entity);
        gizmo_axis.set(TransformComponent::pos(0, 0, -0.5f));
        gizmo_axis.set(EditorGizmoShapeComponent::make_obb(0.1f, 0.1f, 1.0f));
        gizmo_axis.set(
            EditorGizmoDraggableComponent{.drag_axis = Vec3::forward()});
    }

    ecs().world.set<EditorGizmoSelectionData>({});

    ecs()
        .world.system<EditorGizmoSelectionData>("Gizmo Ray")
        .each([this](EditorGizmoSelectionData& data) {
            data.left_mouse_down = ImGui::IsMouseDown(ImGuiMouseButton_Left);
            data.left_mouse_clicked =
                ImGui::IsMouseClicked(ImGuiMouseButton_Left);

            if (!data.dragging || (!data.left_mouse_down)) {
                data.hit_id   = 0;
                data.t        = INFINITY;
                data.dragging = false;
            }
            data.ray   = ray_from_mouse_pos();
            data.mouse = mouse_pos().xy();
        });

    ecs()
        .world
        .system<
            EditorGizmoSelectionData,
            const TransformComponent,
            const EditorGizmoShapeComponent>("Gizmo Ray Hover")
        .term_at(1)
        .singleton()
        .iter([this](
                  flecs::iter&                     it,
                  EditorGizmoSelectionData*        data,
                  const TransformComponent*        transforms,
                  const EditorGizmoShapeComponent* shapes) {
            if (data->dragging) return false;
            for (int i : it) {
                update_gizmo_selection(
                    it.entity(i),
                    data,
                    transforms[i],
                    shapes[i]);
            }
        });

    ecs()
        .world
        .system<
            EditorGizmoSelectionData,
            const TransformComponent,
            EditorGizmoDraggableComponent>("Gizmo Ray Select")
        .term_at(1)
        .singleton()
        .term_at(2)
        .parent()
        .each([this](
                  flecs::entity                  e,
                  EditorGizmoSelectionData&      data,
                  const TransformComponent&      transform,
                  EditorGizmoDraggableComponent& draggable) {
            if ((data.hit_id == e.id()) && (data.left_mouse_clicked)) {
                if (!data.dragging) {
                    draggable.mouse_start = data.mouse;
                    data.dragging         = true;

                    Vec3 axis = normalize(
                        transform.world_rotation * draggable.drag_axis);
                    Vec3 click = data.ray.at(data.t);
                    click =
                        click + project(click - Vec3(transform.position), axis);

                    draggable.click_position = click;
                    draggable.initial_position =
                        Vec3(transform.position) + axis * 0.5f;
                }
            }
        });

    ecs()
        .world
        .system<
            const EditorGizmoSelectionData,
            TransformComponent,
            const EditorGizmoDraggableComponent>("Gizmo Ray Drag")
        .term_at(1)
        .singleton()
        .term_at(2)
        .parent()
        .each([this](
                  flecs::entity                        entity,
                  const EditorGizmoSelectionData&      selection,
                  TransformComponent&                  transform,
                  const EditorGizmoDraggableComponent& draggable) {
            if (!selection.dragging) return;
            if (selection.hit_id != entity.id()) return;

            Vec3 axis_direction =
                normalize(transform.world_rotation * draggable.drag_axis);

            Vec3 click_offset =
                draggable.click_position - draggable.initial_position;

            Vec3 position = Vec3(transform.position) + click_offset;

            Vec3 plane_tangent = normalize(cross(
                axis_direction,
                Vec3(engine().debug_camera.position) - position));
            Vec3 plane_normal = normalize(cross(axis_direction, plane_tangent));

            Plane plane(plane_normal, position);

            float t;
            if (intersect_ray_plane(selection.ray, plane, t)) {
                if (t < 0) return;
                Vec3 initial      = draggable.initial_position + click_offset;
                Vec3 intersection = selection.ray.at(t);

                // engine().imm.box(intersection, Quat::identity(), Vec3(0.1f));

                Vec3 new_pos = intersection;
                new_pos = initial + project(new_pos - initial, axis_direction);

                //  @todo: This assumes that the gizmo is in world space, and
                //  not under some other entity. Update once we can change the
                //  world transform components
                transform.position = new_pos - click_offset;
            }
        });

    editor_world()
        .observer<const EditorSelectableComponent>()
        .event<events::OnEntitySelectionChanged>()
        .oper(flecs::Or)
        .with(flecs::Disabled)
        .each([this](
                  flecs::iter&                     it,
                  size_t                           i,
                  const EditorSelectableComponent& sel) {
            gizmo_entity.set(
                EditorSyncTransformPositionComponent{it.entity(i)});
            const TransformComponent* transform =
                it.entity(i).get<TransformComponent>();
            TransformComponent* sync =
                gizmo_entity.get_mut<TransformComponent>();

            sync->position = transform->world_position;
            sync->rotation = transform->world_rotation;
        });

    editor_world()
        .system<
            const EditorSyncTransformPositionComponent,
            const TransformComponent>()
        .each([this](
                  const EditorSyncTransformPositionComponent& sync,
                  const TransformComponent&                   transform) {
            if (sync.target == 0) {
                return;
            }
            flecs::entity       e(editor_world().m_world, sync.target);
            TransformComponent* et = e.get_mut<TransformComponent>();
            et->position           = transform.position;
        });
}

void WorldViewEditorWindow::draw()
{
    window_pos.x = ImGui::GetWindowPos().x;
    window_pos.y = ImGui::GetWindowPos().y;

    Engine& eng = engine();

    ImVec2 viewport = ImGui::GetContentRegionAvail();
    if (viewport_width != (u32)viewport.x || viewport_height != (u32)viewport.y)
    {
        viewport_width  = viewport.x;
        viewport_height = viewport.y;

        ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)viewport_texture_ref);

        eng.resize_offscreen_buffer(viewport_width, viewport_height);

        viewport_texture_ref = (void*)ImGui_ImplVulkan_AddTexture(
            eng.present_pass.texture_sampler,
            eng.color_pass.color_image_view,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    image_screen_pos = Vec2{
        ImGui::GetCursorScreenPos().x,
        ImGui::GetCursorScreenPos().y,
    };

    ImGui::Image(
        (ImTextureID)viewport_texture_ref,
        ImVec2(viewport_width, viewport_height));

    const EditorGizmoSelectionData* selection =
        ecs().world.get<EditorGizmoSelectionData>();

    ImGui::Begin("Debug");
    ImGui::Text(
        "Ray Origin: %f %f %f",
        selection->ray.origin.x,
        selection->ray.origin.y,
        selection->ray.origin.z);
    ImGui::Text(
        "Ray Direction: %f %f %f",
        selection->ray.direction.x,
        selection->ray.direction.y,
        selection->ray.direction.z);
    ImGui::Text("Hit ID: %u (%f)", selection->hit_id, selection->t);
    ImGui::Text(
        "Dragging: %d (%d)",
        selection->dragging,
        selection->left_mouse_down);

    auto filter =
        ecs()
            .world
            .filter_builder<
                const EditorGizmoShapeComponent,
                const EditorGizmoDraggableComponent,
                const TransformComponent>()
            .term_at(3)
            .parent()
            .build();

    filter.iter([this](
                    flecs::iter&                         it,
                    const EditorGizmoShapeComponent*     shape,
                    const EditorGizmoDraggableComponent* draggable,
                    const TransformComponent*            parent) {
        ImGui::Text(
            "Parent %f %f %f",
            parent->position.x,
            parent->position.y,
            parent->position.z);
        for (auto i : it) {
            ImGui::Text(
                "%s: Original: %f %f %f",
                it.entity(i).name().c_str(),
                draggable[i].initial_position.x,
                draggable[i].initial_position.y,
                draggable[i].initial_position.z);

            ImGui::Text(
                "%s: Start: %f %f %f",
                it.entity(i).name().c_str(),
                draggable[i].click_position.x,
                draggable[i].click_position.y,
                draggable[i].click_position.z);
        }
    });
    ImGui::End();

    flecs::entity entity = editor_world().get_alive(selected_entity);
    if (!entity.is_valid()) return;

    if (!entity.has<TransformComponent>()) return;

    auto* transform = entity.get_mut<TransformComponent>();
}

void WorldViewEditorWindow::deinit()
{
    Engine& eng      = engine();
    eng.do_blit_pass = true;

    eng.resize_offscreen_buffer(eng.extent.width, eng.extent.height);
    ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)viewport_texture_ref);
}

Vec3 WorldViewEditorWindow::mouse_pos()
{
    Vec2 imgui_mouse_pos = {
        ImGui::GetMousePos().x,
        ImGui::GetMousePos().y,
    };

    imgui_mouse_pos.x -= (image_screen_pos.x);
    imgui_mouse_pos.y -= (image_screen_pos.y);

    return Vec3(imgui_mouse_pos, 0.f);
}

void WorldViewEditorWindow::on_change_selection(flecs::entity new_entity)
{
    selected_entity = new_entity;
}

Ray WorldViewEditorWindow::ray_from_mouse_pos()
{
    Engine& eng = engine();

    Vec2 imgui_mouse_pos = mouse_pos().xy();

    Vec2 mpos = {
        +((2.0f * imgui_mouse_pos.x) / float(viewport_width) - 1.0f),
        +((2.0f * imgui_mouse_pos.y) / float(viewport_height) - 1.0f),
    };

    Vec3 ray_nds  = Vec3(mpos.x, mpos.y, 1.0f);
    Vec4 ray_clip = Vec4(ray_nds.xy(), -1.0f, 1.0f);

    Mat4 inv_proj = Mat4(eng.debug_camera.proj).inverse();
    Vec4 ray_eye  = inv_proj * ray_clip;
    ray_eye       = Vec4(ray_eye.xy(), -1.0f, 0.0f);

    Mat4 inv_view  = Mat4(eng.debug_camera.view).inverse();
    Vec3 ray_world = normalize(inv_view * ray_eye.xyz());

    return Ray{
        .origin    = eng.debug_camera.position,
        .direction = ray_world,
    };
}

void WorldViewEditorWindow::begin_style()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
}

void WorldViewEditorWindow::end_style() { ImGui::PopStyleVar(); }

STATIC_MENU_ITEM(
    "Window/World", { The_Editor.create_window<WorldViewEditorWindow>(); }, 0);
