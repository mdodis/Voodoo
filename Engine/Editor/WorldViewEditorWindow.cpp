#include "WorldViewEditorWindow.h"

#include "Engine.h"
#include "Gizmos.h"
#include "MenuRegistrar.h"
#include "backends/imgui_impl_vulkan.h"
#include "imgui.h"

void WorldViewEditorWindow::begin_style()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
}

void WorldViewEditorWindow::end_style() { ImGui::PopStyleVar(); }

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
            selected_entity = it.entity(i);
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
    flecs::entity entity = editor_world().get_alive(selected_entity);
    if (!entity.is_valid()) return;

    if (!entity.has<TransformComponent>()) return;

    auto* transform = entity.get_mut<TransformComponent>();

    Vec3 position = transform->position;
    ed::gizmos::translation(position, Quat(transform->rotation));
    transform->position = position;
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

STATIC_MENU_ITEM(
    "Window/World", { The_Editor.create_window<WorldViewEditorWindow>(); }, 0);
