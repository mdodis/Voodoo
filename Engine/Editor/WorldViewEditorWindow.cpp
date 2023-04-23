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

    {
        auto gizmo_axis = ecs().world.entity("X").child_of(gizmo_entity);
        gizmo_axis.set(TransformComponent::pos(0.5f, 0, 0));
        gizmo_axis.set(EditorGizmoShapeComponent::make_obb(1.0f, 0.1f, 0.1f));
        gizmo_axis.set(EditorGizmoDraggableComponent{});
    }
    {
        auto gizmo_axis = ecs().world.entity("Y").child_of(gizmo_entity);
        gizmo_axis.set(TransformComponent::pos(0, 0.5f, 0));
        gizmo_axis.set(EditorGizmoShapeComponent::make_obb(0.1f, 1.0f, 0.1f));
        gizmo_axis.set(EditorGizmoDraggableComponent{});
    }
    {
        auto gizmo_axis = ecs().world.entity("Z").child_of(gizmo_entity);
        gizmo_axis.set(TransformComponent::pos(0, 0, -0.5f));
        gizmo_axis.set(EditorGizmoShapeComponent::make_obb(0.1f, 0.1f, 1.0f));
        gizmo_axis.set(EditorGizmoDraggableComponent{});
    }

    ecs().world.set<EditorGizmoSelectionData>({});

    ecs()
        .world.system<EditorGizmoSelectionData>("Gizmo Ray")
        .each([this](EditorGizmoSelectionData& data) {
            data.ray             = ray_from_mouse_pos();
            data.hit_id          = 0;
            data.t               = INFINITY;
            data.mouse           = mouse_pos().xy();
            data.left_mouse_down = ImGui::IsMouseDown(ImGuiMouseButton_Left);
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
        .system<const EditorGizmoSelectionData, EditorGizmoDraggableComponent>(
            "Gizmo Ray Select")
        .term_at(1)
        .singleton()
        .each([this](
                  flecs::entity                   e,
                  const EditorGizmoSelectionData& data,
                  EditorGizmoDraggableComponent&  draggable) {
            if ((data.hit_id == e.id()) && (data.left_mouse_down)) {
                if (!draggable.dragging) {
                    draggable.mouse_start = data.mouse;
                    draggable.dragging    = true;
                    print(LIT("Begin Drag! {}\n"), Str(e.name().c_str()));
                }
            } else {
                draggable.dragging = false;
            }
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
